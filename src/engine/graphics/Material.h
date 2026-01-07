#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <string>
#include <memory>

namespace Henky3D {

using namespace DirectX;

// Handle for texture resources
struct TextureHandle {
    uint32_t Index = 0xFFFFFFFF; // Index into texture registry
    bool IsValid() const { return Index != 0xFFFFFFFF; }
};

// Texture asset with DX12 resource and SRV
struct TextureAsset {
    std::wstring Path;
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
    D3D12_CPU_DESCRIPTOR_HANDLE SRV{};
    D3D12_GPU_DESCRIPTOR_HANDLE SRVGPU{};
    uint32_t Width = 0;
    uint32_t Height = 0;
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    bool IsDefault = false; // True for fallback textures
};

// Material asset with PBR parameters
struct MaterialAsset {
    std::string Name = "Unnamed";
    
    // Base color
    XMFLOAT4 BaseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    TextureHandle BaseColorTexture;
    
    // Normal mapping
    TextureHandle NormalTexture;
    
    // Roughness/Metalness (R=metalness, G=roughness in texture)
    float RoughnessFactor = 0.5f;
    float MetalnessFactor = 0.0f;
    TextureHandle RoughnessMetalnessTexture;
    
    // Alpha mode
    bool AlphaMask = false;
    float AlphaCutoff = 0.5f;
    
    // Shader permutation flags
    bool HasBaseColorTexture() const { return BaseColorTexture.IsValid(); }
    bool HasNormalTexture() const { return NormalTexture.IsValid(); }
    bool HasRoughnessMetalnessTexture() const { return RoughnessMetalnessTexture.IsValid(); }
};

// Material component for ECS
struct Material {
    uint32_t MaterialIndex = 0; // Index into material registry
};

} // namespace Henky3D
