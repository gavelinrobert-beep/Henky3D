#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace Henky3D {

// Handle for texture resources
struct TextureHandle {
    uint32_t Index = 0xFFFFFFFF; // Index into texture registry
    bool IsValid() const { return Index != 0xFFFFFFFF; }
};

// Texture asset with OpenGL texture
struct TextureAsset {
    std::wstring Path;
    GLuint Texture = 0;
    uint32_t Width = 0;
    uint32_t Height = 0;
    GLenum Format = GL_RGBA8;
    bool IsDefault = false; // True for fallback textures
};

// Material asset with PBR parameters
struct MaterialAsset {
    std::string Name = "Unnamed";
    
    // Base color
    glm::vec4 BaseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
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
