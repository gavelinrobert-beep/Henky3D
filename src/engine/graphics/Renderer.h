#pragma once
#include "GraphicsDevice.h"
#include "ConstantBuffers.h"
#include "AssetRegistry.h"
#include "ShadowMap.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace Henky3D {

class ECSWorld;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec4 Color;
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
    void DrawCube(const glm::mat4& worldMatrix, const glm::vec4& color);
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
    void CreateShaderPrograms();
    void CreateCubeGeometry();
    GLuint LoadAndCompileShader(const char* filename, GLenum shaderType);
    GLuint CreateShaderProgram(const char* vsFile, const char* fsFile);
    std::string LoadShaderSource(const char* filename);

    GraphicsDevice* m_Device;
    std::unique_ptr<AssetRegistry> m_AssetRegistry;
    std::unique_ptr<ShadowMap> m_ShadowMap;
    
    // Shader programs
    GLuint m_ForwardProgram;
    GLuint m_DepthPrepassProgram;
    GLuint m_ShadowProgram;
    
    // Cube geometry
    GLuint m_CubeVAO;
    GLuint m_CubeVBO;
    GLuint m_CubeIBO;
    GLuint m_IndexCount;
    
    // Uniform buffers
    GLuint m_PerFrameUBO;
    GLuint m_PerDrawUBO;
    
    PerFrameConstants m_PerFrameConstants;
    bool m_DepthPrepassEnabled;
    bool m_ShadowsEnabled;
    
    RenderStats m_Stats;
};

} // namespace Henky3D
