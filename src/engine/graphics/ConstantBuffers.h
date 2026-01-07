#pragma once
#include <DirectXMath.h>

namespace Henky3D {

using namespace DirectX;

// Per-frame constants updated once per frame
struct alignas(256) PerFrameConstants {
    XMFLOAT4X4 ViewMatrix;
    XMFLOAT4X4 ProjectionMatrix;
    XMFLOAT4X4 ViewProjectionMatrix;
    XMFLOAT4X4 LightViewProjectionMatrix;
    XMFLOAT4 CameraPosition;
    XMFLOAT4 LightDirection;  // w component unused
    XMFLOAT4 LightColor;      // rgb = color, a = intensity
    XMFLOAT4 AmbientColor;    // rgb = color, a = intensity
    float Time;
    float DeltaTime;
    float ShadowBias;
    float ShadowsEnabled;  // 1.0 = enabled, 0.0 = disabled
};

// Per-draw constants updated for each draw call
struct alignas(256) PerDrawConstants {
    XMFLOAT4X4 WorldMatrix;
    UINT MaterialIndex;
    UINT Padding[3];
};

} // namespace Henky3D
