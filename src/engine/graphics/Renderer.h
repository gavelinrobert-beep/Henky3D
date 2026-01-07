#pragma once
#include "GraphicsDevice.h"
#include "ConstantBufferAllocator.h"
#include "ConstantBuffers.h"
#include "AssetRegistry.h"
#include "ShadowMap.h"
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

namespace Henky3D {

class ECSWorld;

struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT4 Color;
};

struct RenderStats {
    uint32_t DrawCount = 0;
    uint32_t CulledCount = 0;
    uint32_t TriangleCount = 0;
};

class Renderer {
public:
    Renderer(GraphicsDevice* device);
    ~Renderer();

    void BeginFrame();
    void SetPerFrameConstants(const PerFrameConstants& constants);
    void DrawCube(const DirectX::XMMATRIX& worldMatrix, const DirectX::XMFLOAT4& color);
    void RenderScene(ECSWorld* world, bool enableDepthPrepass, bool enableShadows);
    void RenderShadowPass(ECSWorld* world);

    bool GetDepthPrepassEnabled() const { return m_DepthPrepassEnabled; }
    void SetDepthPrepassEnabled(bool enabled) { m_DepthPrepassEnabled = enabled; }
    
    bool GetShadowsEnabled() const { return m_ShadowsEnabled; }
    void SetShadowsEnabled(bool enabled) { m_ShadowsEnabled = enabled; }
    
    const RenderStats& GetStats() const { return m_Stats; }
    AssetRegistry* GetAssetRegistry() { return m_AssetRegistry.get(); }
    ShadowMap* GetShadowMap() { return m_ShadowMap.get(); }

private:
    void CreateRootSignature();
    void CreateShadowRootSignature();
    void CreateDepthPrepassPipeline();
    void CreateForwardPipeline();
    void CreateShadowPipeline();
    void CreateCubeGeometry();
    void CreateSamplers();
    ComPtr<IDxcBlob> LoadAndCompileShader(const wchar_t* filename, const wchar_t* entryPoint, const wchar_t* target);

    GraphicsDevice* m_Device;
    std::unique_ptr<ConstantBufferAllocator> m_CBAllocator;
    std::unique_ptr<AssetRegistry> m_AssetRegistry;
    std::unique_ptr<ShadowMap> m_ShadowMap;
    
    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12RootSignature> m_ShadowRootSignature;
    ComPtr<ID3D12PipelineState> m_DepthPrepassPSO;
    ComPtr<ID3D12PipelineState> m_ForwardPSO;
    ComPtr<ID3D12PipelineState> m_ForwardNoPrepassPSO;
    ComPtr<ID3D12PipelineState> m_ShadowPSO;
    
    ComPtr<ID3D12Resource> m_VertexBuffer;
    ComPtr<ID3D12Resource> m_IndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
    UINT m_IndexCount;

    D3D12_GPU_VIRTUAL_ADDRESS m_PerFrameCBAddress;
    bool m_DepthPrepassEnabled;
    bool m_ShadowsEnabled;
    
    RenderStats m_Stats;
    
    // Samplers
    ComPtr<ID3D12DescriptorHeap> m_SamplerHeap;
    D3D12_GPU_DESCRIPTOR_HANDLE m_ShadowSamplerGPU;
};

} // namespace Henky3D
