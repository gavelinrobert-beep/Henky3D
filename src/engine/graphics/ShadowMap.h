#pragma once
#include "GraphicsDevice.h"
#include <glad/gl.h>
#include <glm/glm.hpp>

namespace Henky3D {

class ShadowMap {
public:
    ShadowMap(GraphicsDevice* device, uint32_t resolution = 2048);
    ~ShadowMap();

    void Resize(uint32_t resolution);
    void BeginShadowPass();
    void EndShadowPass();
    
    GLuint GetDepthTexture() const { return m_DepthTexture; }
    GLuint GetFramebuffer() const { return m_Framebuffer; }
    uint32_t GetResolution() const { return m_Resolution; }
    
    // Compute light view-projection matrix for directional light
    static glm::mat4 ComputeLightViewProjection(const glm::vec3& lightDirection, 
                                                const glm::vec3& sceneBoundsMin,
                                                const glm::vec3& sceneBoundsMax);

private:
    void CreateResources();
    void DestroyResources();
    
    GraphicsDevice* m_Device;
    uint32_t m_Resolution;
    
    GLuint m_Framebuffer;
    GLuint m_DepthTexture;
};

} // namespace Henky3D
