#pragma once

#include "Engine/Scene/Entity.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ember
{
class Scene
{
public:
    explicit Scene(std::string name = "Untitled Scene");

    [[nodiscard]] const std::string& GetName() const { return m_name; }
    void SetName(std::string name);

    // Returns a stable reference. Other entity insertions do not invalidate it.
    Entity& CreateEntity(std::string name = "Entity");
    [[nodiscard]] bool DestroyEntity(EntityId id);

    [[nodiscard]] Entity* FindEntity(EntityId id);
    [[nodiscard]] const Entity* FindEntity(EntityId id) const;

    [[nodiscard]] std::size_t GetEntityCount() const { return m_entities.size(); }
    [[nodiscard]] bool IsEmpty() const { return m_entities.empty(); }
    [[nodiscard]] std::vector<EntityId> GetEntityIds() const;

    // Removes all entities. IDs are not reused during this Scene's lifetime.
    void Clear();

private:
    std::string m_name;
    EntityId m_nextEntityId = 1;
    std::map<EntityId, std::unique_ptr<Entity>> m_entities;
};
} // namespace ember
