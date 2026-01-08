#include "CullingSystem.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Henky3D {

std::vector<entt::entity> CullingSystem::CullEntities(ECSWorld* world, const Frustum& frustum) {
    std::vector<entt::entity> visibleEntities;
    
    auto& registry = world->GetRegistry();
    auto view = registry.view<Transform, Renderable, BoundingBox>();

    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& renderable = view.get<Renderable>(entity);
        auto& boundingBox = view.get<BoundingBox>(entity);

        // Skip if not visible
        if (!renderable.Visible) {
            continue;
        }

        // Transform bounding box to world space
        glm::mat4 worldMatrix = transform.GetWorldMatrix();
        glm::vec3 localCenter = boundingBox.GetCenter();
        glm::vec3 localExtents = boundingBox.GetExtents();

        // Transform center
        glm::vec4 centerVec4 = worldMatrix * glm::vec4(localCenter, 1.0f);
        glm::vec3 worldCenter = glm::vec3(centerVec4);

        // Transform extents (conservative approach: compute max scale from each axis)
        float scaleX = glm::length(glm::vec3(worldMatrix[0]));
        float scaleY = glm::length(glm::vec3(worldMatrix[1]));
        float scaleZ = glm::length(glm::vec3(worldMatrix[2]));
        float maxScale = std::max({scaleX, scaleY, scaleZ});

        glm::vec3 worldExtents = localExtents * maxScale;

        // Test against frustum
        if (frustum.TestBox(worldCenter, worldExtents)) {
            visibleEntities.push_back(entity);
        }
    }

    return visibleEntities;
}

} // namespace Henky3D
