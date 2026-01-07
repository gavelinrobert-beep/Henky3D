#pragma once
#include <DirectXMath.h>

namespace Henky3D {

using namespace DirectX;

struct Transform {
    XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };

    XMMATRIX GetMatrix() const {
        XMMATRIX translation = XMMatrixTranslation(Position.x, Position.y, Position.z);
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
        XMMATRIX scale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
        return scale * rotation * translation;
    }
};

struct Camera {
    XMFLOAT3 Position = { 0.0f, 0.0f, -5.0f };
    XMFLOAT3 Target = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Up = { 0.0f, 1.0f, 0.0f };
    
    float FOV = XM_PIDIV4;
    float AspectRatio = 16.0f / 9.0f;
    float NearPlane = 0.1f;
    float FarPlane = 1000.0f;
    
    float Yaw = 0.0f;
    float Pitch = 0.0f;
    float MoveSpeed = 5.0f;
    float LookSpeed = 0.002f;

    XMMATRIX GetViewMatrix() const {
        XMVECTOR pos = XMLoadFloat3(&Position);
        XMVECTOR target = XMLoadFloat3(&Target);
        XMVECTOR up = XMLoadFloat3(&Up);
        return XMMatrixLookAtLH(pos, target, up);
    }

    XMMATRIX GetProjectionMatrix() const {
        return XMMatrixPerspectiveFovLH(FOV, AspectRatio, NearPlane, FarPlane);
    }

    void UpdateTargetFromAngles() {
        float x = cosf(Pitch) * sinf(Yaw);
        float y = sinf(Pitch);
        float z = cosf(Pitch) * cosf(Yaw);
        
        Target.x = Position.x + x;
        Target.y = Position.y + y;
        Target.z = Position.z + z;
    }
};

struct Renderable {
    bool Visible = true;
    XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct Light {
    enum class Type {
        Directional,
        Point,
        Spot
    };

    Type LightType = Type::Directional;
    XMFLOAT3 Position = { 0.0f, 5.0f, 0.0f };
    XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
    XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;
    float Range = 10.0f;
};

} // namespace Henky3D
