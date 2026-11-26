#pragma once

#include "Engine/Scene/Entity.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ember
{
class SceneSerializer;

class Scene
{
public:
    explicit Scene(std::string name = "Untitled Scene");
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&& other) noexcept;
    Scene& operator=(Scene&& other) noexcept;

    [[nodiscard]] const std::string& GetName() const { return m_name; }
    void SetName(std::string name);

    void Start() noexcept;
    void Update(float deltaSeconds) noexcept;
    void Stop() noexcept;
    [[nodiscard]] bool IsStarted() const { return m_started; }

    Entity& CreateEntity(std::string name = "Entity");
    [[nodiscard]] bool DestroyEntity(EntityId id);
    [[nodiscard]] Entity* DuplicateEntity(EntityId id);
    [[nodiscard]] Entity* FindEntity(EntityId id);
    [[nodiscard]] const Entity* FindEntity(EntityId id) const;
    [[nodiscard]] std::size_t GetEntityCount() const { return m_entities.size(); }
    [[nodiscard]] bool IsEmpty() const { return m_entities.empty(); }
    [[nodiscard]] std::vector<EntityId> GetEntityIds() const;

    template <typename Function>
    void ForEachEntity(Function&& function)
    {
        for (auto& [id, entity] : m_entities)
        {
            (void)id;
            function(*entity);
        }
    }

    template <typename Function>
    void ForEachEntity(Function&& function) const
    {
        for (const auto& [id, entity] : m_entities)
        {
            (void)id;
            function(*entity);
        }
    }

    void Clear();
    [[nodiscard]] Scene Clone() const;

private:
    friend class SceneSerializer;
    void MoveFrom(Scene&& other) noexcept;

    std::string m_name;
    EntityId m_nextEntityId = 1;
    std::map<EntityId, std::unique_ptr<Entity>> m_entities;
    bool m_started = false;
};
} // namespace ember
