#include "Engine/Scene/Scene.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ember
{
Scene::Scene(std::string name) : m_name(std::move(name)) {}
Scene::~Scene() { Stop(); }

Scene::Scene(Scene&& other) noexcept { MoveFrom(std::move(other)); }

Scene& Scene::operator=(Scene&& other) noexcept
{
    if (this != &other)
    {
        Stop();
        m_entities.clear();
        MoveFrom(std::move(other));
    }
    return *this;
}

void Scene::MoveFrom(Scene&& other) noexcept
{
    m_name = std::move(other.m_name);
    m_nextEntityId = other.m_nextEntityId;
    m_entities = std::move(other.m_entities);
    m_started = other.m_started;
    other.m_nextEntityId = 1;
    other.m_started = false;
}

void Scene::SetName(std::string name) { m_name = std::move(name); }

void Scene::Start() noexcept
{
    if (m_started) return;
    m_started = true;
    for (auto& [id, entity] : m_entities)
    {
        (void)id;
        entity->StartComponents();
    }
}

void Scene::Update(float deltaSeconds) noexcept
{
    if (!m_started || !std::isfinite(deltaSeconds)) return;
    const float safeDeltaSeconds = std::max(0.0F, deltaSeconds);
    for (auto& [id, entity] : m_entities)
    {
        (void)id;
        if (entity->IsActive()) entity->UpdateComponents(safeDeltaSeconds);
    }
}

void Scene::Stop() noexcept
{
    if (!m_started) return;
    for (auto it = m_entities.rbegin(); it != m_entities.rend(); ++it)
        it->second->DestroyComponents();
    m_started = false;
}

Entity& Scene::CreateEntity(std::string name)
{
    if (m_started)
        throw std::logic_error("Entities cannot be created while their Scene is running.");
    if (m_nextEntityId == 0)
        throw std::overflow_error("Scene entity ID space has been exhausted.");

    const EntityId id = m_nextEntityId++;
    auto entity = std::unique_ptr<Entity>(new Entity(id, std::move(name)));
    Entity& result = *entity;
    m_entities.emplace(id, std::move(entity));
    return result;
}

bool Scene::DestroyEntity(EntityId id)
{
    if (m_started)
        throw std::logic_error("Entities cannot be destroyed while their Scene is running.");
    return m_entities.erase(id) != 0;
}

Entity* Scene::DuplicateEntity(EntityId id)
{
    const Entity* source = FindEntity(id);
    if (source == nullptr) return nullptr;

    Entity& duplicate = CreateEntity(source->GetName() + " Copy");
    duplicate.m_transform = source->m_transform;
    duplicate.m_renderComponent = source->m_renderComponent;
    duplicate.m_movementComponent = source->m_movementComponent;
    duplicate.m_active = source->m_active;
    source->CopyComponentsTo(duplicate);
    return &duplicate;
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
    if (m_started)
        throw std::logic_error("A running Scene must be stopped before it can be cloned.");

    Scene clone(m_name);
    clone.m_nextEntityId = m_nextEntityId;
    for (const auto& [id, entity] : m_entities)
    {
        auto copiedEntity = std::unique_ptr<Entity>(new Entity(id, entity->m_name));
        copiedEntity->m_transform = entity->m_transform;
        copiedEntity->m_renderComponent = entity->m_renderComponent;
        copiedEntity->m_movementComponent = entity->m_movementComponent;
        copiedEntity->m_active = entity->m_active;
        entity->CopyComponentsTo(*copiedEntity);
        clone.m_entities.emplace(id, std::move(copiedEntity));
    }
    return clone;
}

void Scene::Clear()
{
    Stop();
    m_entities.clear();
}
} // namespace ember
