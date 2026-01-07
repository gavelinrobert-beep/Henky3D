#pragma once
#include "Material.h"
#include "GraphicsDevice.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace Henky3D {

class AssetRegistry {
public:
    AssetRegistry(GraphicsDevice* device);
    ~AssetRegistry();

    // Initialize default/fallback textures
    void InitializeDefaults();

    // Texture management
    TextureHandle LoadTexture(const std::wstring& path);
    TextureHandle GetDefaultWhiteTexture() const { return m_DefaultWhiteTexture; }
    TextureHandle GetDefaultNormalTexture() const { return m_DefaultNormalTexture; }
    TextureHandle GetDefaultRoughnessMetalnessTexture() const { return m_DefaultRoughnessMetalnessTexture; }
    const TextureAsset* GetTexture(TextureHandle handle) const;

    // Material management
    uint32_t CreateMaterial(const MaterialAsset& material);
    const MaterialAsset* GetMaterial(uint32_t index) const;
    MaterialAsset* GetMaterialMutable(uint32_t index);
    uint32_t GetMaterialCount() const { return static_cast<uint32_t>(m_Materials.size()); }

private:
    TextureHandle CreateDefaultTexture(const std::string& name, uint32_t width, uint32_t height, 
                                       const uint8_t* data, DXGI_FORMAT format);
    TextureHandle LoadTextureWIC(const std::wstring& path);
    TextureHandle LoadTextureDDS(const std::wstring& path);
    
    GraphicsDevice* m_Device;
    
    // Texture storage
    std::vector<std::unique_ptr<TextureAsset>> m_Textures;
    std::unordered_map<std::wstring, TextureHandle> m_TextureCache;
    
    // Default textures
    TextureHandle m_DefaultWhiteTexture;
    TextureHandle m_DefaultNormalTexture;
    TextureHandle m_DefaultRoughnessMetalnessTexture;
    
    // Material storage
    std::vector<MaterialAsset> m_Materials;
};

} // namespace Henky3D
