#pragma once
#include "GraphicsDevice.h"
#include "ConstantBufferAllocator.h"
#include "ConstantBuffers.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

namespace Henky3D {

class ECSWorld;

struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT4 Color;
};

class Renderer {
public:
    Renderer(GraphicsDevice* device);
    ~Renderer();

    void BeginFrame();
    void SetPerFrameConstants(const PerFrameConstants& constants);
    void DrawCube(const DirectX::XMMATRIX& worldMatrix, const DirectX::XMFLOAT4& color);
    void RenderScene(ECSWorld* world, bool enableDepthPrepass);

    bool GetDepthPrepassEnabled() const { return m_DepthPrepassEnabled; }
    void SetDepthPrepassEnabled(bool enabled) { m_DepthPrepassEnabled = enabled; }

private:
    void CreateRootSignature();
    void CreateDepthPrepassPipeline();
    void CreateForwardPipeline();
    void CreateCubeGeometry();
    ComPtr<IDxcBlob> LoadAndCompileShader(const wchar_t* filename, const wchar_t* entryPoint, const wchar_t* target);

    GraphicsDevice* m_Device;
    std::unique_ptr<ConstantBufferAllocator> m_CBAllocator;
    
    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_DepthPrepassPSO;
    ComPtr<ID3D12PipelineState> m_ForwardPSO;
    ComPtr<ID3D12PipelineState> m_ForwardNoPrepassPSO;
    
    ComPtr<ID3D12Resource> m_VertexBuffer;
    ComPtr<ID3D12Resource> m_IndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
    UINT m_IndexCount;

    D3D12_GPU_VIRTUAL_ADDRESS m_PerFrameCBAddress;
    bool m_DepthPrepassEnabled;
};

} // namespace Henky3D
