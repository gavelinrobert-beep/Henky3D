#pragma once
#include <glm/glm.hpp>
#include <cstdint>

namespace Henky3D {

// Per-frame constants updated once per frame
struct alignas(16) PerFrameConstants {
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewProjectionMatrix;
    glm::mat4 LightViewProjectionMatrix;
    glm::vec4 CameraPosition;
    glm::vec4 LightDirection;  // w component unused
    glm::vec4 LightColor;      // rgb = color, a = intensity
    glm::vec4 AmbientColor;    // rgb = color, a = intensity
    float Time;
    float DeltaTime;
    float ShadowBias;
    float ShadowsEnabled;  // 1.0 = enabled, 0.0 = disabled
};

// Per-draw constants updated for each draw call
struct alignas(16) PerDrawConstants {
    glm::mat4 WorldMatrix;
    uint32_t MaterialIndex;
    uint32_t Padding[3];
};

} // namespace Henky3D
