// Common.hlsli - Shared structures and constants for all shaders

#ifndef COMMON_HLSLI
#define COMMON_HLSLI

// Per-frame constants updated once per frame
struct PerFrameConstants {
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 ViewProjectionMatrix;
    float4x4 LightViewProjectionMatrix;
    float4 CameraPosition;
    float4 LightDirection;
    float4 LightColor;
    float4 AmbientColor;
    float Time;
    float DeltaTime;
    float ShadowBias;
    float ShadowsEnabled;
};

// Per-draw constants updated for each draw call
struct PerDrawConstants {
    float4x4 WorldMatrix;
    uint MaterialIndex;
    uint3 Padding;
};

#endif // COMMON_HLSLI
