#include "Renderer.h"
#include "../ecs/ECSWorld.h"
#include "../ecs/Components.h"
#include <dxcapi.h>
#include <DirectXMath.h>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "dxcompiler.lib")

using namespace DirectX;

namespace Henky3D {

static std::wstring GetShaderPath(const wchar_t* filename) {
    // Shaders are in the shaders/ directory relative to executable
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        path = path.substr(0, lastSlash + 1);
    }
    
    // Navigate to project root and then to shaders
    path += L"..\\..\\..\\shaders\\";
    path += filename;
    return path;
}

ComPtr<IDxcBlob> Renderer::LoadAndCompileShader(const wchar_t* filename, const wchar_t* entryPoint, const wchar_t* target) {
    static Microsoft::WRL::ComPtr<IDxcLibrary> library;
    static Microsoft::WRL::ComPtr<IDxcCompiler> compiler;
    static Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;

    if (!library) {
        if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)))) {
            throw std::runtime_error("Failed to create DXC library");
        }
    }

    if (!compiler) {
        if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)))) {
            throw std::runtime_error("Failed to create DXC compiler");
        }
    }

    if (!includeHandler) {
        if (FAILED(library->CreateIncludeHandler(&includeHandler))) {
            throw std::runtime_error("Failed to create DXC include handler");
        }
    }

    // Load shader file
    std::wstring shaderPath = GetShaderPath(filename);
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
    if (FAILED(library->CreateBlobFromFile(shaderPath.c_str(), nullptr, &sourceBlob))) {
        std::wstring error = L"Failed to load shader file: " + shaderPath;
        throw std::runtime_error(std::string(error.begin(), error.end()));
    }

    // Compile shader
    Microsoft::WRL::ComPtr<IDxcOperationResult> result;
    HRESULT hr = compiler->Compile(
        sourceBlob.Get(),
        filename,
        entryPoint,
        target,
        nullptr,
        0,
        nullptr,
        0,
        includeHandler.Get(),
        &result);

    if (FAILED(hr)) {
        throw std::runtime_error("DXC compile invocation failed");
    }

    HRESULT status = S_OK;
    result->GetStatus(&status);
    if (FAILED(status)) {
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> errors;
        result->GetErrorBuffer(&errors);
        std::string message = "DXC compile failed for ";
        std::wstring wfilename(filename);
        message.append(std::string(wfilename.begin(), wfilename.end()));
        if (errors && errors->GetBufferPointer() && errors->GetBufferSize() > 0) {
            message.append(": ");
            message.append(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
        }
        throw std::runtime_error(message);
    }

    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
    result->GetResult(&shaderBlob);
    return shaderBlob;
}

Renderer::Renderer(GraphicsDevice* device) 
    : m_Device(device), m_PerFrameCBAddress(0), m_DepthPrepassEnabled(true) {
    
    m_CBAllocator = std::make_unique<ConstantBufferAllocator>(device);
    
    CreateRootSignature();
    CreateDepthPrepassPipeline();
    CreateForwardPipeline();
    CreateCubeGeometry();
}

Renderer::~Renderer() {
}

void Renderer::CreateRootSignature() {
    // Root parameters: CBV for per-frame constants, CBV for per-draw constants
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    
    // Per-frame constants (b0)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    
    // Per-draw constants (b1)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 1;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = 2;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr))) {
        throw std::runtime_error("Failed to serialize root signature");
    }
    if (FAILED(m_Device->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), 
        signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)))) {
        throw std::runtime_error("Failed to create root signature");
    }
}

void Renderer::CreateDepthPrepassPipeline() {
    // Load and compile shaders
    ComPtr<IDxcBlob> vertexShader = LoadAndCompileShader(L"DepthPrepass.vs.hlsl", L"VSMain", L"vs_6_6");
    ComPtr<IDxcBlob> pixelShader = LoadAndCompileShader(L"DepthPrepass.ps.hlsl", L"PSMain", L"ps_6_6");

    // Input layout for depth prepass (position only)
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_RootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0; // No color write in depth prepass
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 0; // No render targets in depth prepass
    psoDesc.DSVFormat = GraphicsDevice::DepthStencilFormat;
    psoDesc.SampleDesc.Count = 1;

    if (FAILED(m_Device->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_DepthPrepassPSO)))) {
        throw std::runtime_error("Failed to create depth prepass pipeline state");
    }
}

void Renderer::CreateForwardPipeline() {
    // Load and compile shaders
    ComPtr<IDxcBlob> vertexShader = LoadAndCompileShader(L"Forward.vs.hlsl", L"VSMain", L"vs_6_6");
    ComPtr<IDxcBlob> pixelShader = LoadAndCompileShader(L"Forward.ps.hlsl", L"PSMain", L"ps_6_6");

    // Input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
    psoDesc.DepthStencilState.DepthWriteMask = m_DepthPrepassEnabled ? D3D12_DEPTH_WRITE_MASK_ZERO : D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = m_DepthPrepassEnabled ? D3D12_COMPARISON_FUNC_EQUAL : D3D12_COMPARISON_FUNC_LESS;
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = GraphicsDevice::BackBufferFormat;
    psoDesc.DSVFormat = GraphicsDevice::DepthStencilFormat;
    psoDesc.SampleDesc.Count = 1;

    if (FAILED(m_Device->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_ForwardPSO)))) {
        throw std::runtime_error("Failed to create forward pipeline state");
    }
}

void Renderer::CreateCubeGeometry() {
    // Cube vertices with normals and colors
    Vertex cubeVertices[] = {
        // Front face (red tint)
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.3f, 0.3f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.3f, 0.3f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.3f, 0.3f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.3f, 0.3f, 1.0f } },
        
        // Back face (green tint)
        { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.3f, 1.0f, 0.3f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.3f, 1.0f, 0.3f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.3f, 1.0f, 0.3f, 1.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.3f, 1.0f, 0.3f, 1.0f } },
        
        // Left face (blue tint)
        { { -0.5f, -0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.3f, 0.3f, 1.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.3f, 0.3f, 1.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.3f, 0.3f, 1.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.3f, 0.3f, 1.0f, 1.0f } },
        
        // Right face (yellow tint)
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.3f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.3f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.3f, 1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.3f, 1.0f } },
        
        // Top face (cyan tint)
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.3f, 1.0f, 1.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.3f, 1.0f, 1.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.3f, 1.0f, 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.3f, 1.0f, 1.0f, 1.0f } },
        
        // Bottom face (magenta tint)
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.3f, 1.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.3f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.3f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.3f, 1.0f, 1.0f } }
    };

    UINT16 cubeIndices[] = {
        0, 1, 2, 0, 2, 3,       // Front
        4, 5, 6, 4, 6, 7,       // Back
        8, 9, 10, 8, 10, 11,    // Left
        12, 13, 14, 12, 14, 15, // Right
        16, 17, 18, 16, 18, 19, // Top
        20, 21, 22, 20, 22, 23  // Bottom
    };

    m_IndexCount = _countof(cubeIndices);

    // Create vertex buffer
    const UINT vertexBufferSize = sizeof(cubeVertices);

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

    if (FAILED(m_Device->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_VertexBuffer)))) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    UINT8* pVertexDataBegin;
    D3D12_RANGE readRange = { 0, 0 };
    m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, cubeVertices, vertexBufferSize);
    m_VertexBuffer->Unmap(0, nullptr);

    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.StrideInBytes = sizeof(Vertex);
    m_VertexBufferView.SizeInBytes = vertexBufferSize;

    // Create index buffer
    const UINT indexBufferSize = sizeof(cubeIndices);

    resourceDesc.Width = indexBufferSize;

    if (FAILED(m_Device->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_IndexBuffer)))) {
        throw std::runtime_error("Failed to create index buffer");
    }

    UINT8* pIndexDataBegin;
    m_IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
    memcpy(pIndexDataBegin, cubeIndices, indexBufferSize);
    m_IndexBuffer->Unmap(0, nullptr);

    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_IndexBufferView.SizeInBytes = indexBufferSize;
}

void Renderer::BeginFrame() {
    m_CBAllocator->Reset();
}

void Renderer::SetPerFrameConstants(const PerFrameConstants& constants) {
    void* cpuAddress;
    m_PerFrameCBAddress = m_CBAllocator->Allocate(sizeof(PerFrameConstants), &cpuAddress);
    memcpy(cpuAddress, &constants, sizeof(PerFrameConstants));
}

void Renderer::DrawCube(const XMMATRIX& worldMatrix, const XMFLOAT4& color) {
    auto commandList = m_Device->GetCommandList();

    // Allocate and fill per-draw constants
    PerDrawConstants drawConstants;
    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&drawConstants.WorldMatrix), XMMatrixTranspose(worldMatrix));
    drawConstants.MaterialIndex = 0;

    void* cpuAddress;
    D3D12_GPU_VIRTUAL_ADDRESS drawCBAddress = m_CBAllocator->Allocate(sizeof(PerDrawConstants), &cpuAddress);
    memcpy(cpuAddress, &drawConstants, sizeof(PerDrawConstants));

    // Set root signature and constants
    commandList->SetGraphicsRootSignature(m_RootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(0, m_PerFrameCBAddress);
    commandList->SetGraphicsRootConstantBufferView(1, drawCBAddress);

    // Set geometry
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    commandList->IASetIndexBuffer(&m_IndexBufferView);

    // Draw
    commandList->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}

void Renderer::RenderScene(ECSWorld* world, bool enableDepthPrepass) {
    auto commandList = m_Device->GetCommandList();

    // Get visible entities (for now, render all with Transform + Renderable)
    auto view = world->GetRegistry().view<Transform, Renderable>();

    if (enableDepthPrepass && m_DepthPrepassEnabled) {
        // Depth prepass
        commandList->SetPipelineState(m_DepthPrepassPSO.Get());

        for (auto entity : view) {
            auto& transform = view.get<Transform>(entity);
            auto& renderable = view.get<Renderable>(entity);

            if (!renderable.Visible) continue;

            DrawCube(transform.GetMatrix(), renderable.Color);
        }
    }

    // Forward pass
    commandList->SetPipelineState(m_ForwardPSO.Get());

    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& renderable = view.get<Renderable>(entity);

        if (!renderable.Visible) continue;

        DrawCube(transform.GetMatrix(), renderable.Color);
    }
}

} // namespace Henky3D

} // namespace Henky3D
