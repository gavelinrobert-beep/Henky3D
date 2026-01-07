#pragma once
#include <DirectXMath.h>

namespace Henky3D {

using namespace DirectX;

// Per-frame constants updated once per frame
struct alignas(256) PerFrameConstants {
    XMFLOAT4X4 ViewMatrix;
    XMFLOAT4X4 ProjectionMatrix;
    XMFLOAT4X4 ViewProjectionMatrix;
    XMFLOAT4 CameraPosition;
    float Time;
    float DeltaTime;
    float Padding[2];
};

// Per-draw constants updated for each draw call
struct alignas(256) PerDrawConstants {
    XMFLOAT4X4 WorldMatrix;
    UINT MaterialIndex;
    UINT Padding[3];
};

} // namespace Henky3D
