// Depth Prepass Vertex Shader
#version 460 core

#include "Common.glsl"

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;

void main() {
    vec4 worldPos = WorldMatrix * vec4(aPosition, 1.0);
    gl_Position = ViewProjectionMatrix * worldPos;
}
