#include "CullingSystem.h"
#include <DirectXMath.h>

using namespace DirectX;

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
        XMMATRIX worldMatrix = transform.GetMatrix();
        XMFLOAT3 localCenter = boundingBox.GetCenter();
        XMFLOAT3 localExtents = boundingBox.GetExtents();

        // Transform center
        XMVECTOR centerVec = XMLoadFloat3(&localCenter);
        centerVec = XMVector3Transform(centerVec, worldMatrix);
        XMFLOAT3 worldCenter;
        XMStoreFloat3(&worldCenter, centerVec);

        // Transform extents (conservative approach: compute max scale from each axis)
        float scaleX = XMVectorGetX(XMVector3Length(worldMatrix.r[0]));
        float scaleY = XMVectorGetX(XMVector3Length(worldMatrix.r[1]));
        float scaleZ = XMVectorGetX(XMVector3Length(worldMatrix.r[2]));
        float maxScale = std::max({scaleX, scaleY, scaleZ});

        XMFLOAT3 worldExtents = {
            localExtents.x * maxScale,
            localExtents.y * maxScale,
            localExtents.z * maxScale
        };

        // Test against frustum
        if (frustum.TestBox(worldCenter, worldExtents)) {
            visibleEntities.push_back(entity);
        }
    }

    return visibleEntities;
}

} // namespace Henky3D
