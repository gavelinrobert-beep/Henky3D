// Common.glsl - Shared structures and constants for all shaders

#ifndef COMMON_GLSL
#define COMMON_GLSL

// Per-frame constants updated once per frame
layout(std140, binding = 0) uniform PerFrameConstants {
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
    mat4 ViewProjectionMatrix;
    mat4 LightViewProjectionMatrix;
    vec4 CameraPosition;
    vec4 LightDirection;
    vec4 LightColor;
    vec4 AmbientColor;
    float Time;
    float DeltaTime;
    float ShadowBias;
    float ShadowsEnabled;
};

// Per-draw constants updated for each draw call
layout(std140, binding = 1) uniform PerDrawConstants {
    mat4 WorldMatrix;
    uint MaterialIndex;
};

#endif // COMMON_GLSL
