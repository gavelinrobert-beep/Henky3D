#pragma once
#include <entt/entt.hpp>

namespace Henky3D {

class ECSWorld {
public:
    ECSWorld() = default;

    entt::registry& GetRegistry() { return m_Registry; }
    const entt::registry& GetRegistry() const { return m_Registry; }

    entt::entity CreateEntity() {
        return m_Registry.create();
    }

    void DestroyEntity(entt::entity entity) {
        m_Registry.destroy(entity);
    }

    template<typename Component, typename... Args>
    Component& AddComponent(entt::entity entity, Args&&... args) {
        return m_Registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    Component& GetComponent(entt::entity entity) {
        return m_Registry.get<Component>(entity);
    }

    template<typename Component>
    bool HasComponent(entt::entity entity) const {
        return m_Registry.all_of<Component>(entity);
    }

    template<typename Component>
    void RemoveComponent(entt::entity entity) {
        m_Registry.remove<Component>(entity);
    }

    void Update(float deltaTime) {
        // Simple update order stub
        // In a real engine, this would call various systems in order:
        // 1. Physics update
        // 2. Transform hierarchy update
        // 3. Animation update
        // 4. Render system prepare
        // etc.
    }

private:
    entt::registry m_Registry;
};

} // namespace Henky3D
