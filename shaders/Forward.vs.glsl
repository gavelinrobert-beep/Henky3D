// Forward Pass Vertex Shader
#version 460 core

#include "Common.glsl"

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;

out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vColor;
out vec4 vShadowPos;

void main() {
    vec4 worldPos = WorldMatrix * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = ViewProjectionMatrix * worldPos;
    
    // Transform normal to world space (assuming uniform scale)
    vNormal = mat3(WorldMatrix) * aNormal;
    vColor = aColor;
    
    // Compute shadow map coordinates
    vShadowPos = LightViewProjectionMatrix * worldPos;
}
