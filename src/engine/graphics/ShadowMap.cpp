#include "ShadowMap.h"
#include <d3dx12.h>
#include <stdexcept>

namespace Henky3D {

ShadowMap::ShadowMap(GraphicsDevice* device, uint32_t resolution)
    : m_Device(device), m_Resolution(resolution) {
    CreateResources();
}

ShadowMap::~ShadowMap() {
}

void ShadowMap::Resize(uint32_t resolution) {
    m_Resolution = resolution;
    CreateResources();
}

void ShadowMap::CreateResources() {
    // Create depth texture for shadow map
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = m_Resolution;
    depthDesc.Height = m_Resolution;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Typeless for both DSV and SRV
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    if (FAILED(m_Device->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_DepthTexture)))) {
        throw std::runtime_error("Failed to create shadow map depth texture");
    }

    // Create DSV
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    m_DSV = m_Device->AllocateDsvDescriptor();
    m_Device->GetDevice()->CreateDepthStencilView(m_DepthTexture.Get(), &dsvDesc, m_DSV);
    
    // Create SRV for sampling in shader
    auto srvHandle = m_Device->AllocateSrvDescriptor(1);
    m_SRV = srvHandle.CPUHandle;
    m_SRVGPU = srvHandle.GPUHandle;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    m_Device->GetDevice()->CreateShaderResourceView(m_DepthTexture.Get(), &srvDesc, m_SRV);
}

XMMATRIX ShadowMap::ComputeLightViewProjection(const XMFLOAT3& lightDirection,
                                                 const XMFLOAT3& sceneBoundsMin,
                                                 const XMFLOAT3& sceneBoundsMax) {
    // Compute scene center
    XMVECTOR center = XMLoadFloat3(&sceneBoundsMin);
    center = XMVectorAdd(center, XMLoadFloat3(&sceneBoundsMax));
    center = XMVectorScale(center, 0.5f);

    // Compute light position (place light far from scene in opposite direction)
    XMVECTOR lightDir = XMLoadFloat3(&lightDirection);
    lightDir = XMVector3Normalize(lightDir);
    XMVECTOR lightPos = XMVectorSubtract(center, XMVectorScale(lightDir, 50.0f));

    // Create light view matrix
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    // If light direction is nearly vertical, use different up vector
    float dotUp = XMVectorGetY(lightDir);
    if (fabs(dotUp) > 0.99f) {
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, center, up);

    // Compute orthographic projection that encompasses the scene
    // Transform scene bounds to light space
    XMFLOAT3 corners[8] = {
        { sceneBoundsMin.x, sceneBoundsMin.y, sceneBoundsMin.z },
        { sceneBoundsMax.x, sceneBoundsMin.y, sceneBoundsMin.z },
        { sceneBoundsMin.x, sceneBoundsMax.y, sceneBoundsMin.z },
        { sceneBoundsMax.x, sceneBoundsMax.y, sceneBoundsMin.z },
        { sceneBoundsMin.x, sceneBoundsMin.y, sceneBoundsMax.z },
        { sceneBoundsMax.x, sceneBoundsMin.y, sceneBoundsMax.z },
        { sceneBoundsMin.x, sceneBoundsMax.y, sceneBoundsMax.z },
        { sceneBoundsMax.x, sceneBoundsMax.y, sceneBoundsMax.z }
    };

    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;

    for (int i = 0; i < 8; i++) {
        XMVECTOR corner = XMLoadFloat3(&corners[i]);
        corner = XMVector3Transform(corner, lightView);
        
        float x = XMVectorGetX(corner);
        float y = XMVectorGetY(corner);
        float z = XMVectorGetZ(corner);

        minX = (std::min)(minX, x);
        maxX = (std::max)(maxX, x);
        minY = (std::min)(minY, y);
        maxY = (std::max)(maxY, y);
        minZ = (std::min)(minZ, z);
        maxZ = (std::max)(maxZ, z);
    }

    // Add some padding to avoid edge artifacts
    float padding = 2.0f;
    minX -= padding;
    maxX += padding;
    minY -= padding;
    maxY += padding;
    minZ -= padding;
    maxZ += padding;

    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(minX, maxX, minY, maxY, minZ, maxZ);

    return lightView * lightProj;
}

} // namespace Henky3D
