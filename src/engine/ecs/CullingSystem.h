#pragma once
#include "ECSWorld.h"
#include "Components.h"
#include <vector>
#include <entt/entt.hpp>

namespace Henky3D {

class CullingSystem {
public:
    // Perform frustum culling and return list of visible entities
    static std::vector<entt::entity> CullEntities(ECSWorld* world, const Frustum& frustum);
};

} // namespace Henky3D
