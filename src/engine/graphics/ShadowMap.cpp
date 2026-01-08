#include "ShadowMap.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <iostream>

namespace Henky3D {

ShadowMap::ShadowMap(GraphicsDevice* device, uint32_t resolution)
    : m_Device(device), m_Resolution(resolution), m_Framebuffer(0), m_DepthTexture(0) {
    CreateResources();
}

ShadowMap::~ShadowMap() {
    DestroyResources();
}

void ShadowMap::CreateResources() {
    // Create framebuffer
    glGenFramebuffers(1, &m_Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    
    // Create depth texture
    glGenTextures(1, &m_DepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_Resolution, m_Resolution, 
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    
    // Set texture parameters for shadow sampling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Enable shadow comparison mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    
    // Attach depth texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);
    
    // No color attachment needed for shadow map
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Shadow map framebuffer is not complete");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    std::cout << "Shadow map created: " << m_Resolution << "x" << m_Resolution << std::endl;
}

void ShadowMap::DestroyResources() {
    if (m_DepthTexture) {
        glDeleteTextures(1, &m_DepthTexture);
        m_DepthTexture = 0;
    }
    if (m_Framebuffer) {
        glDeleteFramebuffers(1, &m_Framebuffer);
        m_Framebuffer = 0;
    }
}

void ShadowMap::Resize(uint32_t resolution) {
    if (m_Resolution == resolution) return;
    
    m_Resolution = resolution;
    DestroyResources();
    CreateResources();
}

void ShadowMap::BeginShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, m_Resolution, m_Resolution);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::EndShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 ShadowMap::ComputeLightViewProjection(const glm::vec3& lightDirection,
                                                 const glm::vec3& sceneBoundsMin,
                                                 const glm::vec3& sceneBoundsMax) {
    // Compute scene center and size
    glm::vec3 sceneCenter = (sceneBoundsMin + sceneBoundsMax) * 0.5f;
    glm::vec3 sceneExtents = sceneBoundsMax - sceneBoundsMin;
    float sceneRadius = glm::length(sceneExtents) * 0.5f;
    
    // Normalize light direction
    glm::vec3 lightDir = glm::normalize(lightDirection);
    
    // Position light to look at scene center
    glm::vec3 lightPos = sceneCenter - lightDir * sceneRadius * 2.0f;
    
    // Compute light view matrix
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Compute orthographic projection that encompasses the scene
    float orthoSize = sceneRadius * 2.0f;
    glm::mat4 lightProjection = glm::ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        0.1f, sceneRadius * 4.0f
    );
    
    return lightProjection * lightView;
}

} // namespace Henky3D
