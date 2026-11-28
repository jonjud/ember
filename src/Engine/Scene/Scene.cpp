#include "Engine/Scene/Scene.hpp"
#include "Engine/Physics/PhysicsComponents2D.hpp"
#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Scene/AudioComponents2D.hpp"
#include "Engine/UI/UIComponents2D.hpp"
#include "Engine/Scripting/ScriptComponent.hpp"

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
    m_logger = other.m_logger;
    m_loggedAudioDiagnostics = other.m_loggedAudioDiagnostics;
    m_loggedPhysicsDiagnostics = other.m_loggedPhysicsDiagnostics;
    m_loggedScriptDiagnostics = other.m_loggedScriptDiagnostics;
    m_nextEntityId = other.m_nextEntityId;
    m_entities = std::move(other.m_entities);
    m_started = other.m_started;
    m_physicsWorld = std::move(other.m_physicsWorld);
    m_audioEngine = other.m_audioEngine;
    m_audioSources = std::move(other.m_audioSources);
    m_scriptRuntime = other.m_scriptRuntime;
    m_scriptRuntimeInitialized = other.m_scriptRuntimeInitialized;
    m_scriptInstances = std::move(other.m_scriptInstances);
    m_scriptDiagnostics = std::move(other.m_scriptDiagnostics);
    m_audioDiagnostics = std::move(other.m_audioDiagnostics);
    m_physicsBodies = std::move(other.m_physicsBodies);
    m_physicsDiagnostics = std::move(other.m_physicsDiagnostics);
    m_physicsContactEvents = std::move(other.m_physicsContactEvents);
    m_activeContactCounts = std::move(other.m_activeContactCounts);
    m_physicsAccumulator = other.m_physicsAccumulator;
    other.m_audioEngine = nullptr;
    other.m_scriptRuntime = nullptr;
    other.m_scriptRuntimeInitialized = false;
    other.m_scriptInstances.clear();
    other.m_audioSources.clear();
    other.m_nextEntityId = 1;
    other.m_started = false;
    other.m_physicsAccumulator = 0.0F;
}

void Scene::SetName(std::string name) { m_name = std::move(name); }

void Scene::SetLogger(Logger* logger) noexcept
{
    m_logger = logger;
    m_loggedAudioDiagnostics = 0;
    m_loggedPhysicsDiagnostics = 0;
    m_loggedScriptDiagnostics = 0;
    FlushDiagnosticsToLogger();
}

void Scene::FlushDiagnosticsToLogger() noexcept
{
    if (m_logger == nullptr) return;

    if (m_loggedAudioDiagnostics > m_audioDiagnostics.size()) m_loggedAudioDiagnostics = 0;
    if (m_loggedPhysicsDiagnostics > m_physicsDiagnostics.size()) m_loggedPhysicsDiagnostics = 0;
    if (m_loggedScriptDiagnostics > m_scriptDiagnostics.size()) m_loggedScriptDiagnostics = 0;

    for (; m_loggedAudioDiagnostics < m_audioDiagnostics.size(); ++m_loggedAudioDiagnostics)
        m_logger->Warning("Audio", m_audioDiagnostics[m_loggedAudioDiagnostics]);
    for (; m_loggedPhysicsDiagnostics < m_physicsDiagnostics.size(); ++m_loggedPhysicsDiagnostics)
        m_logger->Error("Physics", m_physicsDiagnostics[m_loggedPhysicsDiagnostics]);
    for (; m_loggedScriptDiagnostics < m_scriptDiagnostics.size(); ++m_loggedScriptDiagnostics)
        m_logger->Error("Scripting", m_scriptDiagnostics[m_loggedScriptDiagnostics]);
}

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
        const auto* colliderSet = entity->GetComponent<ColliderSet2D>();
        const auto* box = entity->GetComponent<BoxCollider2D>();
        const auto* circle = entity->GetComponent<CircleCollider2D>();
        const bool hasLegacyCollider = box != nullptr || circle != nullptr;
        if (bodyConfig == nullptr)
        {
            if (hasLegacyCollider || colliderSet != nullptr)
                m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                    " has a collider but no PhysicsBody2D; physics was skipped.");
            continue;
        }
        if (!bodyConfig->enabled) continue;
        if ((box != nullptr && circle != nullptr) ||
            (colliderSet != nullptr && hasLegacyCollider))
        {
            m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " has conflicting legacy and ColliderSet2D collider components; physics was skipped.");
            continue;
        }
        if (colliderSet != nullptr && !colliderSet->HasValidColliderIds())
        {
            m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " has invalid or duplicate ColliderSet2D IDs; physics was skipped.");
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

            const auto addCollider = [this, handle, pixelsToMeters](const Collider2D& entry)
            {
                switch (entry.shape)
                {
                case ColliderShape2D::Box:
                {
                    PhysicsBoxColliderDefinition2D collider;
                    collider.halfExtents = {entry.halfExtents.x * pixelsToMeters,
                        entry.halfExtents.y * pixelsToMeters};
                    collider.offset = {entry.offset.x * pixelsToMeters,
                        entry.offset.y * pixelsToMeters};
                    collider.rotation = entry.rotation;
                    collider.density = entry.density;
                    collider.friction = entry.friction;
                    collider.restitution = entry.restitution;
                    collider.isSensor = entry.isSensor;
                    collider.categoryBits = entry.categoryBits;
                    collider.maskBits = entry.maskBits;
                    return m_physicsWorld->AddBoxCollider(handle, collider);
                }
                case ColliderShape2D::Circle:
                {
                    PhysicsCircleColliderDefinition2D collider;
                    collider.radius = entry.radius * pixelsToMeters;
                    collider.offset = {entry.offset.x * pixelsToMeters,
                        entry.offset.y * pixelsToMeters};
                    collider.density = entry.density;
                    collider.friction = entry.friction;
                    collider.restitution = entry.restitution;
                    collider.isSensor = entry.isSensor;
                    collider.categoryBits = entry.categoryBits;
                    collider.maskBits = entry.maskBits;
                    return m_physicsWorld->AddCircleCollider(handle, collider);
                }
                }
                return false;
            };

            bool colliderCreated = true;
            if (colliderSet != nullptr)
            {
                for (const Collider2D& entry : colliderSet->colliders)
                {
                    if (!addCollider(entry))
                    {
                        colliderCreated = false;
                        break;
                    }
                }
            }
            else if (box != nullptr)
            {
                Collider2D entry(ColliderShape2D::Box, 1);
                entry.halfExtents = box->halfExtents;
                entry.offset = box->offset;
                entry.rotation = box->rotation;
                entry.density = box->density;
                entry.friction = box->friction;
                entry.restitution = box->restitution;
                entry.isSensor = box->isSensor;
                colliderCreated = addCollider(entry);
            }
            else if (circle != nullptr)
            {
                Collider2D entry(ColliderShape2D::Circle, 1);
                entry.radius = circle->radius;
                entry.offset = circle->offset;
                entry.density = circle->density;
                entry.friction = circle->friction;
                entry.restitution = circle->restitution;
                entry.isSensor = circle->isSensor;
                colliderCreated = addCollider(entry);
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

    // Resolve Entity references only after all runtime bodies have been created.
    for (const auto& [entityId, entity] : m_entities)
    {
        const auto ownerBody = m_physicsBodies.find(entityId);
        if (const auto* joint = entity->GetComponent<RevoluteJoint2D>())
        {
            const auto targetBody = m_physicsBodies.find(joint->connectedEntity);
            if (joint->connectedEntity == entityId || ownerBody == m_physicsBodies.end() ||
                targetBody == m_physicsBodies.end())
            {
                m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                    " has a RevoluteJoint2D with a missing, self, disabled, or invalid body reference.");
            }
            else
            {
                PhysicsRevoluteJointDefinition2D definition;
                definition.bodyA = ownerBody->second;
                definition.bodyB = targetBody->second;
                definition.localAnchorA = {joint->localAnchorA.x * pixelsToMeters,
                    joint->localAnchorA.y * pixelsToMeters};
                definition.localAnchorB = {joint->localAnchorB.x * pixelsToMeters,
                    joint->localAnchorB.y * pixelsToMeters};
                definition.enableLimit = joint->enableLimit;
                definition.lowerAngle = joint->lowerAngle;
                definition.upperAngle = joint->upperAngle;
                definition.enableMotor = joint->enableMotor;
                definition.motorSpeed = joint->motorSpeed;
                definition.maxMotorTorque = joint->maxMotorTorque;
                definition.collideConnected = joint->collideConnected;
                if (!m_physicsWorld->CreateRevoluteJoint(definition))
                    m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                        " could not create its RevoluteJoint2D.");
            }
        }
        if (const auto* joint = entity->GetComponent<DistanceJoint2D>())
        {
            const auto targetBody = m_physicsBodies.find(joint->connectedEntity);
            if (joint->connectedEntity == entityId || ownerBody == m_physicsBodies.end() ||
                targetBody == m_physicsBodies.end())
            {
                m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                    " has a DistanceJoint2D with a missing, self, disabled, or invalid body reference.");
            }
            else
            {
                PhysicsDistanceJointDefinition2D definition;
                definition.bodyA = ownerBody->second;
                definition.bodyB = targetBody->second;
                definition.localAnchorA = {joint->localAnchorA.x * pixelsToMeters,
                    joint->localAnchorA.y * pixelsToMeters};
                definition.localAnchorB = {joint->localAnchorB.x * pixelsToMeters,
                    joint->localAnchorB.y * pixelsToMeters};
                definition.length = joint->length * pixelsToMeters;
                definition.collideConnected = joint->collideConnected;
                if (!m_physicsWorld->CreateDistanceJoint(definition))
                    m_physicsDiagnostics.push_back("Entity " + std::to_string(entityId) +
                        " could not create its DistanceJoint2D.");
            }
        }
    }
}

void Scene::DestroyPhysicsWorld() noexcept
{
    m_physicsBodies.clear();
    m_physicsContactEvents.clear();
    m_activeContactCounts.clear();
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
        if (bodyConfig == nullptr) continue;

        const Transform2D& transform = entity->GetTransform();
        const PhysicsTransform2D desired{
            {transform.position.x / metersToPixels, transform.position.y / metersToPixels},
            transform.rotation
        };

        if (bodyConfig->type == PhysicsBodyType2D::Static ||
            bodyConfig->type == PhysicsBodyType2D::Kinematic)
        {
            (void)m_physicsWorld->SetBodyTransform(handle, desired);
        }
        else if (bodyConfig->type == PhysicsBodyType2D::Dynamic)
        {
            // Gameplay/scripts may directly edit a dynamic Entity transform.
            // Only teleport when the Entity actually diverged from the last
            // physics transform; otherwise simulation remains authoritative.
            const auto bodyTransform = m_physicsWorld->GetBodyTransform(handle);
            if (bodyTransform &&
                (std::abs(bodyTransform->position.x - desired.position.x) > 0.0001F ||
                 std::abs(bodyTransform->position.y - desired.position.y) > 0.0001F ||
                 std::abs(bodyTransform->rotation - desired.rotation) > 0.0001F))
                (void)m_physicsWorld->SetBodyTransform(handle, desired);
        }
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
        for (const PhysicsContactEvent2D& event : m_physicsWorld->ConsumeContactEvents())
        {
            const auto findEntityId = [this](PhysicsBodyHandle handle) -> EntityId
            {
                const auto it = std::find_if(m_physicsBodies.begin(), m_physicsBodies.end(),
                    [handle](const auto& entry) { return entry.second == handle; });
                return it != m_physicsBodies.end() ? it->first : 0;
            };
            const EntityId entityA = findEntityId(event.bodyA);
            const EntityId entityB = findEntityId(event.bodyB);
            if (entityA == 0 || entityB == 0) continue;
            const std::pair<EntityId, EntityId> entityPair{
                std::min(entityA, entityB), std::max(entityA, entityB)};
            if (event.type == PhysicsContactEventType2D::Begin)
            {
                std::size_t& activeCount = m_activeContactCounts[entityPair];
                if (activeCount == 0)
                    m_physicsContactEvents.push_back({SceneContactEventType2D::Begin,
                        entityPair.first, entityPair.second});
                ++activeCount;
            }
            else
            {
                const auto active = m_activeContactCounts.find(entityPair);
                if (active == m_activeContactCounts.end()) continue;
                if (active->second > 0) --active->second;
                if (active->second == 0)
                {
                    m_physicsContactEvents.push_back({SceneContactEventType2D::End,
                        entityPair.first, entityPair.second});
                    m_activeContactCounts.erase(active);
                }
            }
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

std::optional<SceneRaycastHit2D> Scene::RaycastClosest(
    Vector2 origin, Vector2 translation, const PhysicsQueryFilter2D& filter) const
{
    if (!m_started || m_physicsWorld == nullptr) return std::nullopt;
    constexpr float scale = 0.01F;
    const auto hit = m_physicsWorld->RaycastClosest(
        {origin.x * scale, origin.y * scale},
        {translation.x * scale, translation.y * scale}, filter);
    if (!hit) return std::nullopt;
    const auto entity = std::find_if(m_physicsBodies.begin(), m_physicsBodies.end(),
        [&hit](const auto& entry) { return entry.second == hit->body; });
    if (entity == m_physicsBodies.end()) return std::nullopt;
    constexpr float inverseScale = 100.0F;
    return SceneRaycastHit2D{entity->first,
        {hit->point.x * inverseScale, hit->point.y * inverseScale}, hit->normal, hit->fraction};
}

std::vector<EntityId> Scene::OverlapAABB(
    Vector2 lowerBound, Vector2 upperBound, const PhysicsQueryFilter2D& filter) const
{
    if (!m_started || m_physicsWorld == nullptr) return {};
    constexpr float scale = 0.01F;
    const auto bodies = m_physicsWorld->OverlapAABB(
        {lowerBound.x * scale, lowerBound.y * scale},
        {upperBound.x * scale, upperBound.y * scale}, filter);
    std::vector<EntityId> entities;
    for (const auto body : bodies)
    {
        const auto entity = std::find_if(m_physicsBodies.begin(), m_physicsBodies.end(),
            [body](const auto& entry) { return entry.second == body; });
        if (entity != m_physicsBodies.end()) entities.push_back(entity->first);
    }
    std::sort(entities.begin(), entities.end());
    entities.erase(std::unique(entities.begin(), entities.end()), entities.end());
    return entities;
}

void Scene::Start(AudioEngine* audioEngine, const AssetRegistry* assets,
    ScriptRuntime* scriptRuntime) noexcept
{
    if (m_started) return;
    m_started = true;
    m_audioEngine = audioEngine;
    m_scriptDiagnostics.clear();
    m_scriptRuntime = scriptRuntime;
    if (m_scriptRuntime != nullptr)
    {
        try
        {
            m_scriptRuntimeInitialized = m_scriptRuntime->Initialize(&m_scriptDiagnostics.emplace_back());
            if (!m_scriptRuntimeInitialized)
            {
                if (m_scriptDiagnostics.back().empty())
                    m_scriptDiagnostics.back() = "ScriptRuntime initialization failed.";
                m_scriptRuntime = nullptr;
            }
            else if (m_scriptDiagnostics.back().empty())
                m_scriptDiagnostics.pop_back();
        }
        catch (const std::exception& exception)
        {
            m_scriptDiagnostics.push_back(std::string("ScriptRuntime initialization threw: ") + exception.what());
            m_scriptRuntime = nullptr;
        }
        catch (...)
        {
            m_scriptDiagnostics.push_back("ScriptRuntime initialization threw an unknown exception.");
            m_scriptRuntime = nullptr;
        }
    }
    for (auto& [id, entity] : m_entities)
    {
        (void)id;
        entity->StartComponents();
    }
    BuildPhysicsWorld();
    BuildAudioSources(assets);
    BuildScriptInstances();
    FlushDiagnosticsToLogger();
}


void Scene::BuildScriptInstances() noexcept
{
    if (m_scriptRuntime == nullptr || !m_scriptRuntimeInitialized) return;
    for (auto& [entityId, entity] : m_entities)
    {
        if (!entity->IsActive()) continue;
        for (const auto& base : entity->GetComponents())
        {
            auto* component = dynamic_cast<ScriptComponent*>(base.get());
            if (component == nullptr || !component->enabled) continue;
            if (!component->IsConfigurationValid())
            {
                m_scriptDiagnostics.push_back("Entity \"" + entity->GetName() + "\" (" + std::to_string(entityId) +
                    ") has an invalid ScriptComponent configuration.");
                continue;
            }
            if (component->scriptAssetId.empty()) continue;
            try
            {
                std::string error;
                const auto instance = m_scriptRuntime->CreateInstance(*component, *entity, &error);
                if (!instance || !static_cast<bool>(*instance))
                {
                    m_scriptDiagnostics.push_back("Entity \"" + entity->GetName() + "\" (" + std::to_string(entityId) +
                        ") could not create script '" + component->scriptAssetId + "': " +
                        (error.empty() ? "backend returned no instance." : error));
                    continue;
                }
                if (!m_scriptRuntime->InvokeOnCreate(*instance, *entity, &error))
                {
                    m_scriptRuntime->InvokeOnDestroy(*instance, *entity);
                    m_scriptDiagnostics.push_back("Entity \"" + entity->GetName() + "\" (" + std::to_string(entityId) +
                        ") OnCreate failed for script '" + component->scriptAssetId + "': " +
                        (error.empty() ? "backend reported failure." : error));
                    continue;
                }
                m_scriptInstances.push_back({entityId, component, *instance, false});
            }
            catch (const std::exception& exception)
            {
                m_scriptDiagnostics.push_back("Entity \"" + entity->GetName() + "\" (" + std::to_string(entityId) +
                    ") script setup threw: " + exception.what());
            }
            catch (...)
            {
                m_scriptDiagnostics.push_back("Entity \"" + entity->GetName() + "\" (" + std::to_string(entityId) +
                    ") script setup threw an unknown exception.");
            }
        }
    }
}

void Scene::UpdateScriptInstances(float deltaSeconds, const InputActionState& input) noexcept
{
    if (m_scriptRuntime == nullptr) return;
    for (auto& instance : m_scriptInstances)
    {
        if (instance.faulted || instance.component == nullptr || !instance.component->enabled) continue;
        Entity* entity = FindEntity(instance.entity);
        if (entity == nullptr || !entity->IsActive()) continue;
        try
        {
            std::string error;
            const ScriptExecutionContext context{*entity, deltaSeconds, input};
            if (!m_scriptRuntime->InvokeOnUpdate(instance.handle, context, &error))
            {
                instance.faulted = true;
                m_scriptDiagnostics.push_back("Entity \"" + entity->GetName() + "\" (" + std::to_string(instance.entity) +
                    ") script '" + instance.component->scriptAssetId + "' OnUpdate failed: " +
                    (error.empty() ? "backend reported failure; instance disabled." : error));
            }
        }
        catch (const std::exception& exception)
        {
            instance.faulted = true;
            m_scriptDiagnostics.push_back("Entity " + std::to_string(instance.entity) +
                " script OnUpdate threw: " + exception.what());
        }
        catch (...)
        {
            instance.faulted = true;
            m_scriptDiagnostics.push_back("Entity " + std::to_string(instance.entity) +
                " script OnUpdate threw an unknown exception.");
        }
    }
}

void Scene::DestroyScriptInstances() noexcept
{
    if (m_scriptRuntime != nullptr)
    {
        for (auto it = m_scriptInstances.rbegin(); it != m_scriptInstances.rend(); ++it)
        {
            if (Entity* entity = FindEntity(it->entity))
                m_scriptRuntime->InvokeOnDestroy(it->handle, *entity);
        }
    }
    m_scriptInstances.clear();
    if (m_scriptRuntime != nullptr && m_scriptRuntimeInitialized)
        m_scriptRuntime->Shutdown();
    m_scriptRuntime = nullptr;
    m_scriptRuntimeInitialized = false;
}

void Scene::BuildAudioSources(const AssetRegistry* assets) noexcept
{
    m_audioDiagnostics.clear();

    for (const auto& [entityId, entity] : m_entities)
    {
        const auto* source = entity->GetComponent<AudioSource2D>();
        if (source == nullptr || !source->enabled || !entity->IsActive()) continue;
        if (m_audioEngine == nullptr)
        {
            m_audioDiagnostics.push_back("AudioEngine was not supplied; Entity " +
                std::to_string(entityId) + " audio was skipped.");
            continue;
        }
        if (!source->IsConfigurationValid() || source->audioAssetId.empty())
        {
            m_audioDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " has an invalid AudioSource2D configuration.");
            continue;
        }
        if (!m_audioEngine->IsInitialized())
        {
            m_audioDiagnostics.push_back("AudioEngine is unavailable; Entity " +
                std::to_string(entityId) + " audio was skipped.");
            continue;
        }
        if (assets == nullptr)
        {
            m_audioDiagnostics.push_back("No AssetRegistry was supplied; Entity " +
                std::to_string(entityId) + " audio was skipped.");
            continue;
        }

        const AssetMetadata* metadata = assets->Find(source->audioAssetId);
        if (metadata == nullptr || metadata->type != AssetType::Audio)
        {
            m_audioDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " references a missing or non-Audio asset: " + source->audioAssetId);
            continue;
        }
        const auto resolvedPath = assets->GetDirectory().Resolve(metadata->source);
        if (!resolvedPath)
        {
            m_audioDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " references an unsafe Audio asset path.");
            continue;
        }

        const AudioSoundHandle sound = m_audioEngine->LoadSound(*resolvedPath, source->streaming);
        if (!sound)
        {
            m_audioDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " could not load Audio asset: " + source->audioAssetId);
            continue;
        }
        if (!m_audioEngine->SetVolume(sound, source->volume) ||
            !m_audioEngine->SetLooping(sound, source->loop) ||
            (source->playOnStart && !m_audioEngine->Play(sound)))
        {
            (void)m_audioEngine->UnloadSound(sound);
            m_audioDiagnostics.push_back("Entity " + std::to_string(entityId) +
                " could not configure or start its AudioSource2D.");
            continue;
        }
        m_audioSources.emplace(entityId, sound);
    }
}

void Scene::DestroyAudioSources() noexcept
{
    if (m_audioEngine != nullptr)
    {
        for (const auto& [entityId, sound] : m_audioSources)
        {
            (void)entityId;
            (void)m_audioEngine->Stop(sound);
            (void)m_audioEngine->UnloadSound(sound);
        }
    }
    m_audioSources.clear();
    m_audioEngine = nullptr;
}

bool Scene::PlayAudioSource(EntityId entity) noexcept
{
    const auto it = m_audioSources.find(entity);
    return it != m_audioSources.end() && m_audioEngine != nullptr &&
        m_audioEngine->SeekToStart(it->second) && m_audioEngine->Play(it->second);
}

bool Scene::StopAudioSource(EntityId entity) noexcept
{
    const auto it = m_audioSources.find(entity);
    return it != m_audioSources.end() && m_audioEngine != nullptr &&
        m_audioEngine->Stop(it->second);
}

bool Scene::IsAudioSourcePlaying(EntityId entity) const noexcept
{
    const auto it = m_audioSources.find(entity);
    return it != m_audioSources.end() && m_audioEngine != nullptr &&
        m_audioEngine->IsPlaying(it->second);
}

bool Scene::DispatchUIButtonClicked(EntityId entityId) noexcept
{
    if (!m_started) return false;
    Entity* entity = FindEntity(entityId);
    if (entity == nullptr || !entity->IsActive()) return false;

    const auto* button = entity->GetComponent<UIButton2D>();
    if (button == nullptr || !button->enabled || !button->interactable ||
        !button->IsConfigurationValid())
        return false;

    for (const auto& component : entity->GetComponents())
        component->OnUIButtonClicked(*entity);
    return true;
}

void Scene::Update(float deltaSeconds) noexcept
{
    UpdateInternal(deltaSeconds, nullptr);
}

void Scene::Update(float deltaSeconds, const InputActionState& input) noexcept
{
    UpdateInternal(deltaSeconds, &input);
}

void Scene::UpdateInternal(float deltaSeconds, const InputActionState* input) noexcept
{
    if (!m_started || !std::isfinite(deltaSeconds)) return;
    const float safeDeltaSeconds = std::max(0.0F, deltaSeconds);
    static const InputActionState emptyInput{};
    const InputActionState& scriptInput = input != nullptr ? *input : emptyInput;
    for (auto& [id, entity] : m_entities)
    {
        (void)id;
        if (!entity->IsActive()) continue;
        if (input != nullptr) entity->UpdateComponents(safeDeltaSeconds, *input);
        else entity->UpdateComponents(safeDeltaSeconds);
    }
    UpdateScriptInstances(safeDeltaSeconds, scriptInput);
    m_physicsContactEvents.clear();
    StepPhysics(safeDeltaSeconds);
    FlushDiagnosticsToLogger();
}

void Scene::Stop() noexcept
{
    if (!m_started) return;
    DestroyScriptInstances();
    DestroyAudioSources();
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
