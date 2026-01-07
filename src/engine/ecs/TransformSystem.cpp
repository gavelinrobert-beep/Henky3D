#include "TransformSystem.h"
#include <DirectXMath.h>

using namespace DirectX;

namespace Henky3D {

void TransformSystem::UpdateTransforms(ECSWorld* world) {
    auto& registry = world->GetRegistry();
    auto view = registry.view<Transform>();

    // Update all root transforms (those without a parent)
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        
        // Skip if this transform has a parent (it will be updated recursively)
        if (transform.Parent != entt::null) {
            continue;
        }

        // Update this transform and its children
        UpdateTransformRecursive(world, entity, XMMatrixIdentity());
    }
}

void TransformSystem::UpdateTransformRecursive(ECSWorld* world, entt::entity entity, const XMMATRIX& parentWorld) {
    auto& registry = world->GetRegistry();
    
    if (!registry.valid(entity) || !registry.any_of<Transform>(entity)) {
        return;
    }

    auto& transform = registry.get<Transform>(entity);

    // Only update if dirty
    if (transform.Dirty) {
        XMMATRIX localMatrix = transform.GetLocalMatrix();
        XMMATRIX worldMatrix = localMatrix * parentWorld;
        XMStoreFloat4x4(&transform.WorldMatrix, worldMatrix);
        transform.Dirty = false;
    }

    // Update all children
    XMMATRIX worldMatrix = XMLoadFloat4x4(&transform.WorldMatrix);
    auto allTransforms = registry.view<Transform>();
    for (auto childEntity : allTransforms) {
        auto& childTransform = allTransforms.get<Transform>(childEntity);
        if (childTransform.Parent == entity) {
            UpdateTransformRecursive(world, childEntity, worldMatrix);
        }
    }
}

} // namespace Henky3D
