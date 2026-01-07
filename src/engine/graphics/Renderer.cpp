#include "Renderer.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

namespace Henky3D {

struct Vertex {
    XMFLOAT3 Position;
    XMFLOAT4 Color;
};

Renderer::Renderer(GraphicsDevice* device) : m_Device(device) {
    CreatePipelineState();
    CreateVertexBuffer();
}

Renderer::~Renderer() {
}

void Renderer::CreatePipelineState() {
    // Root signature
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    m_Device->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), 
        signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));

    // Shader code
    const char* vertexShaderCode = R"(
        struct VSInput {
            float3 position : POSITION;
            float4 color : COLOR;
        };
        
        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };
        
        PSInput VSMain(VSInput input) {
            PSInput output;
            output.position = float4(input.position, 1.0f);
            output.color = input.color;
            return output;
        }
    )";

    const char* pixelShaderCode = R"(
        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };
        
        float4 PSMain(PSInput input) : SV_TARGET {
            return input.color;
        }
    )";

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    
    D3DCompile(vertexShaderCode, strlen(vertexShaderCode), nullptr, nullptr, nullptr,
        "VSMain", "vs_5_1", 0, 0, &vertexShader, &error);
    
    D3DCompile(pixelShaderCode, strlen(pixelShaderCode), nullptr, nullptr, nullptr,
        "PSMain", "ps_5_1", 0, 0, &pixelShader, &error);

    // Input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_RootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = GraphicsDevice::DepthStencilFormat;
    psoDesc.SampleDesc.Count = 1;

    m_Device->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
}

void Renderer::CreateVertexBuffer() {
    Vertex triangleVertices[] = {
        { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };

    const UINT vertexBufferSize = sizeof(triangleVertices);

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = vertexBufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    m_Device->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_VertexBuffer));

    UINT8* pVertexDataBegin;
    D3D12_RANGE readRange = { 0, 0 };
    m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, triangleVertices, vertexBufferSize);
    m_VertexBuffer->Unmap(0, nullptr);

    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.StrideInBytes = sizeof(Vertex);
    m_VertexBufferView.SizeInBytes = vertexBufferSize;
}

void Renderer::RenderTriangle() {
    auto commandList = m_Device->GetCommandList();
    
    commandList->SetGraphicsRootSignature(m_RootSignature.Get());
    commandList->SetPipelineState(m_PipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    commandList->DrawInstanced(3, 1, 0, 0);
}

} // namespace Henky3D
