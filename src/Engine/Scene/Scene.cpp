#include "Engine/Scene/Scene.hpp"
#include "Engine/Physics/PhysicsComponents2D.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
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
    m_physicsWorld = std::move(other.m_physicsWorld);
    m_physicsBodies = std::move(other.m_physicsBodies);
    m_physicsDiagnostics = std::move(other.m_physicsDiagnostics);
    m_physicsAccumulator = other.m_physicsAccumulator;
    other.m_nextEntityId = 1;
    other.m_started = false;
    other.m_physicsAccumulator = 0.0F;
}

void Scene::SetName(std::string name) { m_name = std::move(name); }

void Scene::BuildPhysicsWorld() noexcept
{
    DestroyPhysicsWorld();
    m_physicsDiagnostics.clear();
    try
    {
        m_physicsWorld = std::make_unique<PhysicsWorld>();
    }
    catch (const std::exception& exception)
    {
        m_physicsDiagnostics.push_back(std::string("Could not create PhysicsWorld: ") + exception.what());
        return;
    }

    constexpr float pixelsToMeters = 0.01F;
    for (const auto& [entityId, entity] : m_entities)
    {
        const auto* bodyConfig = entity->GetComponent<PhysicsBody2D>();
        const auto* box = entity->GetComponent<BoxCollider2D>();
        const auto* circle = entity->GetComponent<CircleCollider2D>();
        if (bodyConfig == nullptr)
        {
            if (box != nullptr || circle != nullptr)
                m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                    " has a collider but no PhysicsBody2D; physics was skipped.");
            continue;
        }
        if (!bodyConfig->enabled) continue;
        if (box != nullptr && circle != nullptr)
        {
            m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " has both BoxCollider2D and CircleCollider2D; only one collider is supported.");
            continue;
        }

        PhysicsBodyHandle handle{};
        try
        {
            const Transform2D& transform = entity->GetTransform();
            PhysicsBodyDefinition2D bodyDefinition;
            bodyDefinition.type = bodyConfig->type;
            bodyDefinition.position = {transform.position.x * pixelsToMeters,
                transform.position.y * pixelsToMeters};
            bodyDefinition.rotation = transform.rotation;
            bodyDefinition.gravityScale = bodyConfig->gravityScale;
            bodyDefinition.linearDamping = bodyConfig->linearDamping;
            bodyDefinition.angularDamping = bodyConfig->angularDamping;
            handle = m_physicsWorld->CreateBody(bodyDefinition);
            if (!handle)
            {
                m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                    " could not create its physics body.");
                continue;
            }

            bool colliderCreated = true;
            if (box != nullptr)
            {
                PhysicsBoxColliderDefinition2D collider;
                collider.halfExtents = {box->halfExtents.x * pixelsToMeters,
                    box->halfExtents.y * pixelsToMeters};
                collider.offset = {box->offset.x * pixelsToMeters,
                    box->offset.y * pixelsToMeters};
                collider.rotation = box->rotation;
                collider.density = box->density;
                collider.friction = box->friction;
                collider.restitution = box->restitution;
                collider.isSensor = box->isSensor;
                colliderCreated = m_physicsWorld->AddBoxCollider(handle, collider);
            }
            else if (circle != nullptr)
            {
                PhysicsCircleColliderDefinition2D collider;
                collider.radius = circle->radius * pixelsToMeters;
                collider.offset = {circle->offset.x * pixelsToMeters,
                    circle->offset.y * pixelsToMeters};
                collider.density = circle->density;
                collider.friction = circle->friction;
                collider.restitution = circle->restitution;
                collider.isSensor = circle->isSensor;
                colliderCreated = m_physicsWorld->AddCircleCollider(handle, collider);
            }

            if (!colliderCreated)
            {
                (void)m_physicsWorld->DestroyBody(handle);
                m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                    " has invalid collider configuration; its physics body was discarded.");
                continue;
            }
            m_physicsBodies.emplace(entityId, handle);
        }
        catch (const std::exception& exception)
        {
            if (handle) (void)m_physicsWorld->DestroyBody(handle);
            m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " physics setup failed: " + exception.what());
        }
    }
}

void Scene::DestroyPhysicsWorld() noexcept
{
    m_physicsBodies.clear();
    m_physicsWorld.reset();
    m_physicsAccumulator = 0.0F;
}

void Scene::StepPhysics(float deltaSeconds) noexcept
{
    if (m_physicsWorld == nullptr || !m_physicsWorld->IsValid()) return;
    constexpr float fixedTimeStep = 1.0F / 60.0F;
    constexpr float maxFrameDelta = 0.25F;
    constexpr int maxStepsPerFrame = 8;
    constexpr float maxAccumulatedTime = fixedTimeStep * static_cast<float>(maxStepsPerFrame);
    constexpr float metersToPixels = 100.0F;

    for (const auto& [entityId, handle] : m_physicsBodies)
    {
        const Entity* entity = FindEntity(entityId);
        if (entity == nullptr) continue;
        const auto* bodyConfig = entity->GetComponent<PhysicsBody2D>();
        if (bodyConfig == nullptr || bodyConfig->type == PhysicsBodyType2D::Dynamic) continue;
        const Transform2D& transform = entity->GetTransform();
        (void)m_physicsWorld->SetBodyTransform(handle, {
            {transform.position.x / metersToPixels, transform.position.y / metersToPixels},
            transform.rotation
        });
    }

    const float safeDelta = std::clamp(deltaSeconds, 0.0F, maxFrameDelta);
    m_physicsAccumulator = std::min(m_physicsAccumulator + safeDelta, maxAccumulatedTime);
    int steps = 0;
    while (m_physicsAccumulator >= fixedTimeStep && steps < maxStepsPerFrame)
    {
        if (!m_physicsWorld->Step(fixedTimeStep, 4))
        {
            m_physicsDiagnostics.push_back("PhysicsWorld failed to advance a simulation step.");
            break;
        }
        m_physicsAccumulator -= fixedTimeStep;
        ++steps;
    }

    for (const auto& [entityId, handle] : m_physicsBodies)
    {
        Entity* entity = FindEntity(entityId);
        if (entity == nullptr) continue;
        const auto* bodyConfig = entity->GetComponent<PhysicsBody2D>();
        if (bodyConfig == nullptr || bodyConfig->type != PhysicsBodyType2D::Dynamic) continue;
        const auto transform = m_physicsWorld->GetBodyTransform(handle);
        if (!transform.has_value()) continue;
        entity->GetTransform().position = {transform->position.x * metersToPixels,
            transform->position.y * metersToPixels};
        entity->GetTransform().rotation = transform->rotation;
    }
}

void Scene::Start() noexcept
{
    if (m_started) return;
    m_started = true;
    for (auto& [id, entity] : m_entities)
    {
        (void)id;
        entity->StartComponents();
    }
    BuildPhysicsWorld();
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
    StepPhysics(safeDeltaSeconds);
}

void Scene::Stop() noexcept
{
    if (!m_started) return;
    DestroyPhysicsWorld();
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
