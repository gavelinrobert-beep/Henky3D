#pragma once
#include "ECSWorld.h"
#include "Components.h"

namespace Henky3D {

class TransformSystem {
public:
    // Update all transforms in the hierarchy, computing world matrices
    static void UpdateTransforms(ECSWorld* world);

private:
    static void UpdateTransformRecursive(ECSWorld* world, entt::entity entity, const DirectX::XMMATRIX& parentWorld);
};

} // namespace Henky3D
