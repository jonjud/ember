#include "Engine/Scene/Scene.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace ember
{
Scene::Scene(std::string name)
    : m_name(std::move(name))
{
}

void Scene::SetName(std::string name)
{
    m_name = std::move(name);
}

Entity& Scene::CreateEntity(std::string name)
{
    if (m_nextEntityId == 0)
    {
        throw std::overflow_error("Scene entity ID space has been exhausted.");
    }

    const EntityId id = m_nextEntityId++;
    auto entity = std::unique_ptr<Entity>(new Entity(id, std::move(name)));
    Entity& result = *entity;
    m_entities.emplace(id, std::move(entity));
    return result;
}

bool Scene::DestroyEntity(EntityId id)
{
    return m_entities.erase(id) != 0;
}

Entity* Scene::FindEntity(EntityId id)
{
    const auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

const Entity* Scene::FindEntity(EntityId id) const
{
    const auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

std::vector<EntityId> Scene::GetEntityIds() const
{
    std::vector<EntityId> ids;
    ids.reserve(m_entities.size());

    for (const auto& [id, entity] : m_entities)
    {
        (void)entity;
        ids.push_back(id);
    }

    return ids;
}

Scene Scene::Clone() const
{
    Scene clone(m_name);
    clone.m_nextEntityId = m_nextEntityId;

    for (const auto& [id, entity] : m_entities)
    {
        auto copiedEntity = std::unique_ptr<Entity>(new Entity(id, entity->m_name));
        copiedEntity->m_transform = entity->m_transform;
        copiedEntity->m_renderComponent = entity->m_renderComponent;
        copiedEntity->m_movementComponent = entity->m_movementComponent;
        copiedEntity->m_active = entity->m_active;
        clone.m_entities.emplace(id, std::move(copiedEntity));
    }

    return clone;
}

void Scene::Clear()
{
    m_entities.clear();
}
} // namespace ember
