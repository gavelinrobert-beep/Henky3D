#pragma once
#include <DirectXMath.h>
#include <entt/entt.hpp>

namespace Henky3D {

using namespace DirectX;

// Forward declarations
struct Frustum;

struct Transform {
    XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };

    // Hierarchy support
    entt::entity Parent = entt::null;
    
    // Cached world matrix and dirty flag
    mutable XMFLOAT4X4 WorldMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    mutable bool Dirty = true;

    XMMATRIX GetLocalMatrix() const {
        XMMATRIX translation = XMMatrixTranslation(Position.x, Position.y, Position.z);
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
        XMMATRIX scale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
        return scale * rotation * translation;
    }

    XMMATRIX GetMatrix() const {
        return XMLoadFloat4x4(&WorldMatrix);
    }

    void MarkDirty() {
        Dirty = true;
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

    Frustum GetFrustum() const {
        Frustum frustum;
        XMMATRIX vp = GetViewMatrix() * GetProjectionMatrix();
        frustum.ExtractFromMatrix(vp);
        return frustum;
    }
};

struct Renderable {
    bool Visible = true;
    XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct BoundingBox {
    XMFLOAT3 Min = { -0.5f, -0.5f, -0.5f };
    XMFLOAT3 Max = { 0.5f, 0.5f, 0.5f };

    XMFLOAT3 GetCenter() const {
        return XMFLOAT3(
            (Min.x + Max.x) * 0.5f,
            (Min.y + Max.y) * 0.5f,
            (Min.z + Max.z) * 0.5f
        );
    }

    XMFLOAT3 GetExtents() const {
        return XMFLOAT3(
            (Max.x - Min.x) * 0.5f,
            (Max.y - Min.y) * 0.5f,
            (Max.z - Min.z) * 0.5f
        );
    }
};

struct Frustum {
    XMFLOAT4 Planes[6]; // Left, Right, Bottom, Top, Near, Far
    
    void ExtractFromMatrix(const XMMATRIX& viewProjection) {
        XMFLOAT4X4 vp;
        XMStoreFloat4x4(&vp, viewProjection);
        
        // Left plane
        Planes[0].x = vp._14 + vp._11;
        Planes[0].y = vp._24 + vp._21;
        Planes[0].z = vp._34 + vp._31;
        Planes[0].w = vp._44 + vp._41;
        
        // Right plane
        Planes[1].x = vp._14 - vp._11;
        Planes[1].y = vp._24 - vp._21;
        Planes[1].z = vp._34 - vp._31;
        Planes[1].w = vp._44 - vp._41;
        
        // Bottom plane
        Planes[2].x = vp._14 + vp._12;
        Planes[2].y = vp._24 + vp._22;
        Planes[2].z = vp._34 + vp._32;
        Planes[2].w = vp._44 + vp._42;
        
        // Top plane
        Planes[3].x = vp._14 - vp._12;
        Planes[3].y = vp._24 - vp._22;
        Planes[3].z = vp._34 - vp._32;
        Planes[3].w = vp._44 - vp._42;
        
        // Near plane
        Planes[4].x = vp._13;
        Planes[4].y = vp._23;
        Planes[4].z = vp._33;
        Planes[4].w = vp._43;
        
        // Far plane
        Planes[5].x = vp._14 - vp._13;
        Planes[5].y = vp._24 - vp._23;
        Planes[5].z = vp._34 - vp._33;
        Planes[5].w = vp._44 - vp._43;
        
        // Normalize planes
        for (int i = 0; i < 6; i++) {
            XMVECTOR plane = XMLoadFloat4(&Planes[i]);
            plane = XMPlaneNormalize(plane);
            XMStoreFloat4(&Planes[i], plane);
        }
    }
    
    bool TestBox(const XMFLOAT3& center, const XMFLOAT3& extents) const {
        for (int i = 0; i < 6; i++) {
            XMVECTOR plane = XMLoadFloat4(&Planes[i]);
            XMVECTOR centerVec = XMLoadFloat3(&center);
            XMVECTOR extentsVec = XMLoadFloat3(&extents);
            
            // Compute positive extents along plane normal
            XMVECTOR planeNormal = XMVectorSet(
                XMVectorGetX(plane),
                XMVectorGetY(plane),
                XMVectorGetZ(plane),
                0.0f
            );
            
            XMVECTOR absPlaneNormal = XMVectorAbs(planeNormal);
            float r = XMVectorGetX(XMVector3Dot(extentsVec, absPlaneNormal));
            
            // Compute distance from center to plane
            float d = XMVectorGetX(XMPlaneDotCoord(plane, centerVec));
            
            // If box is completely outside this plane, it's not visible
            if (d < -r) {
                return false;
            }
        }
        return true;
    }
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
