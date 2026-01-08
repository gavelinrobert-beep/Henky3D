#include "AssetRegistry.h"
#include <stdexcept>
#include <iostream>

namespace Henky3D {

AssetRegistry::AssetRegistry(GraphicsDevice* device)
    : m_Device(device) {
}

AssetRegistry::~AssetRegistry() {
    // Cleanup textures
    for (auto& texture : m_Textures) {
        if (texture && texture->Texture) {
            glDeleteTextures(1, &texture->Texture);
        }
    }
}

void AssetRegistry::InitializeDefaults() {
    // Create 1x1 white texture
    uint8_t whitePixel[] = { 255, 255, 255, 255 };
    m_DefaultWhiteTexture = CreateDefaultTexture("DefaultWhite", 1, 1, whitePixel, GL_RGBA8);
    
    // Create 1x1 normal texture (pointing up: 128, 128, 255)
    uint8_t normalPixel[] = { 128, 128, 255, 255 };
    m_DefaultNormalTexture = CreateDefaultTexture("DefaultNormal", 1, 1, normalPixel, GL_RGBA8);
    
    // Create 1x1 roughness/metalness texture (R=metalness=0, G=roughness=128)
    uint8_t rmPixel[] = { 0, 128, 0, 255 };
    m_DefaultRoughnessMetalnessTexture = CreateDefaultTexture("DefaultRM", 1, 1, rmPixel, GL_RGBA8);
    
    std::cout << "Default textures initialized" << std::endl;
}

TextureHandle AssetRegistry::CreateDefaultTexture(const std::string& name, uint32_t width, uint32_t height,
                                                   const uint8_t* data, GLenum format) {
    auto texture = std::make_unique<TextureAsset>();
    texture->Path = std::wstring(name.begin(), name.end());
    texture->Width = width;
    texture->Height = height;
    texture->Format = format;
    texture->IsDefault = true;
    
    // Create OpenGL texture
    glGenTextures(1, &texture->Texture);
    glBindTexture(GL_TEXTURE_2D, texture->Texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    TextureHandle handle;
    handle.Index = static_cast<uint32_t>(m_Textures.size());
    m_Textures.push_back(std::move(texture));
    
    return handle;
}

TextureHandle AssetRegistry::LoadTexture(const std::wstring& path) {
    // Check cache
    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end()) {
        return it->second;
    }
    
    // For now, return default white texture
    // TODO: Implement actual texture loading with stb_image
    std::wcout << L"Warning: Texture loading not implemented, using default: " << path << std::endl;
    return m_DefaultWhiteTexture;
}

TextureHandle AssetRegistry::LoadTextureSTB(const std::wstring& path) {
    // TODO: Implement using stb_image
    return m_DefaultWhiteTexture;
}

const TextureAsset* AssetRegistry::GetTexture(TextureHandle handle) const {
    if (!handle.IsValid() || handle.Index >= m_Textures.size()) {
        return nullptr;
    }
    return m_Textures[handle.Index].get();
}

uint32_t AssetRegistry::CreateMaterial(const MaterialAsset& material) {
    uint32_t index = static_cast<uint32_t>(m_Materials.size());
    m_Materials.push_back(material);
    return index;
}

const MaterialAsset* AssetRegistry::GetMaterial(uint32_t index) const {
    if (index >= m_Materials.size()) {
        return nullptr;
    }
    return &m_Materials[index];
}

MaterialAsset* AssetRegistry::GetMaterialMutable(uint32_t index) {
    if (index >= m_Materials.size()) {
        return nullptr;
    }
    return &m_Materials[index];
}

} // namespace Henky3D
