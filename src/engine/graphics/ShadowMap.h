#pragma once
#include "GraphicsDevice.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

namespace Henky3D {

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class ShadowMap {
public:
    ShadowMap(GraphicsDevice* device, uint32_t resolution = 2048);
    ~ShadowMap();

    void Resize(uint32_t resolution);
    
    ID3D12Resource* GetDepthTexture() const { return m_DepthTexture.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_DSV; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_SRV; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPU() const { return m_SRVGPU; }
    uint32_t GetResolution() const { return m_Resolution; }
    
    // Compute light view-projection matrix for directional light
    static XMMATRIX ComputeLightViewProjection(const XMFLOAT3& lightDirection, 
                                                const XMFLOAT3& sceneBoundsMin,
                                                const XMFLOAT3& sceneBoundsMax);

private:
    void CreateResources();
    
    GraphicsDevice* m_Device;
    uint32_t m_Resolution;
    
    ComPtr<ID3D12Resource> m_DepthTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE m_DSV{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRV{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_SRVGPU{};
};

} // namespace Henky3D
