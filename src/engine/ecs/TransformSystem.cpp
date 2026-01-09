#include "TransformSystem.h"
#include <glm/glm.hpp>

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
        UpdateTransformRecursive(world, entity, glm::mat4(1.0f));
    }
}

void TransformSystem::UpdateTransformRecursive(ECSWorld* world, entt::entity entity, const glm::mat4& parentWorld) {
    auto& registry = world->GetRegistry();
    
    if (!registry.valid(entity) || !registry.any_of<Transform>(entity)) {
        return;
    }

    auto& transform = registry.get<Transform>(entity);

    // Only update if dirty
    if (transform.Dirty) {
        glm::mat4 localMatrix = transform.GetLocalMatrix();
        glm::mat4 worldMatrix = parentWorld * localMatrix;
        transform.WorldMatrix = worldMatrix;
        transform.Dirty = false;
    }

    // Update all children
    glm::mat4 worldMatrix = transform.WorldMatrix;
    auto allTransforms = registry.view<Transform>();
    for (auto childEntity : allTransforms) {
        auto& childTransform = allTransforms.get<Transform>(childEntity);
        if (childTransform.Parent == entity) {
            UpdateTransformRecursive(world, childEntity, worldMatrix);
        }
    }
}

} // namespace Henky3D
