#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <entt/entt.hpp>

namespace Henky3D {

struct Transform {
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

    // Hierarchy support
    entt::entity Parent = entt::null;
    
    // Cached world matrix and dirty flag
    mutable glm::mat4 WorldMatrix = glm::mat4(1.0f);
    mutable bool Dirty = true;

    glm::mat4 GetLocalMatrix() const {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), Position);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(rotation, Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);
        return translation * rotation * scale;
    }

    glm::mat4 GetWorldMatrix() const {
        return WorldMatrix;
    }

    void MarkDirty() {
        Dirty = true;
    }
};

struct Frustum {
    glm::vec4 Planes[6]; // Left, Right, Bottom, Top, Near, Far
    
    void ExtractFromMatrix(const glm::mat4& viewProjection) {
        // Left plane
        Planes[0] = glm::vec4(
            viewProjection[0][3] + viewProjection[0][0],
            viewProjection[1][3] + viewProjection[1][0],
            viewProjection[2][3] + viewProjection[2][0],
            viewProjection[3][3] + viewProjection[3][0]
        );
        
        // Right plane
        Planes[1] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][0],
            viewProjection[1][3] - viewProjection[1][0],
            viewProjection[2][3] - viewProjection[2][0],
            viewProjection[3][3] - viewProjection[3][0]
        );
        
        // Bottom plane
        Planes[2] = glm::vec4(
            viewProjection[0][3] + viewProjection[0][1],
            viewProjection[1][3] + viewProjection[1][1],
            viewProjection[2][3] + viewProjection[2][1],
            viewProjection[3][3] + viewProjection[3][1]
        );
        
        // Top plane
        Planes[3] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][1],
            viewProjection[1][3] - viewProjection[1][1],
            viewProjection[2][3] - viewProjection[2][1],
            viewProjection[3][3] - viewProjection[3][1]
        );
        
        // Near plane
        Planes[4] = glm::vec4(
            viewProjection[0][2],
            viewProjection[1][2],
            viewProjection[2][2],
            viewProjection[3][2]
        );
        
        // Far plane
        Planes[5] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][2],
            viewProjection[1][3] - viewProjection[1][2],
            viewProjection[2][3] - viewProjection[2][2],
            viewProjection[3][3] - viewProjection[3][2]
        );
        
        // Normalize planes
        for (int i = 0; i < 6; i++) {
            float length = glm::length(glm::vec3(Planes[i]));
            Planes[i] /= length;
        }
    }
    
    bool TestBox(const glm::vec3& center, const glm::vec3& extents) const {
        for (int i = 0; i < 6; i++) {
            glm::vec3 planeNormal = glm::vec3(Planes[i]);
            glm::vec3 absPlaneNormal = glm::abs(planeNormal);
            float r = glm::dot(extents, absPlaneNormal);
            
            // Compute distance from center to plane
            float d = glm::dot(glm::vec3(Planes[i]), center) + Planes[i].w;
            
            // If box is completely outside this plane, it's not visible
            if (d < -r) {
                return false;
            }
        }
        return true;
    }
};

struct Camera {
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, -5.0f);
    glm::vec3 Target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    float FOV = glm::pi<float>() / 4.0f;
    float AspectRatio = 16.0f / 9.0f;
    float NearPlane = 0.1f;
    float FarPlane = 1000.0f;
    
    float Yaw = 0.0f;
    float Pitch = 0.0f;
    float MoveSpeed = 5.0f;
    float LookSpeed = 0.002f;

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(Position, Target, Up);
    }

    glm::mat4 GetProjectionMatrix() const {
        return glm::perspective(FOV, AspectRatio, NearPlane, FarPlane);
    }

    void UpdateTargetFromAngles() {
        float x = cosf(Pitch) * sinf(Yaw);
        float y = sinf(Pitch);
        float z = cosf(Pitch) * cosf(Yaw);
        
        Target = Position + glm::vec3(x, y, z);
    }

    Frustum GetFrustum() const {
        Frustum frustum;
        glm::mat4 vp = GetProjectionMatrix() * GetViewMatrix();
        frustum.ExtractFromMatrix(vp);
        return frustum;
    }
};

struct Renderable {
    bool Visible = true;
    glm::vec4 Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

struct BoundingBox {
    glm::vec3 Min = glm::vec3(-0.5f, -0.5f, -0.5f);
    glm::vec3 Max = glm::vec3(0.5f, 0.5f, 0.5f);

    glm::vec3 GetCenter() const {
        return (Min + Max) * 0.5f;
    }

    glm::vec3 GetExtents() const {
        return (Max - Min) * 0.5f;
    }
};

struct Light {
    enum class Type {
        Directional,
        Point,
        Spot
    };

    Type LightType = Type::Directional;
    glm::vec3 Position = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3 Direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec4 Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float Intensity = 1.0f;
    float Range = 10.0f;
};

} // namespace Henky3D
