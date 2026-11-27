#pragma once

#include "Engine/Scene/Entity.hpp"
#include "Engine/Physics/PhysicsWorld.hpp"
#include "Engine/Audio/AudioEngine.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ember
{
class AssetRegistry;
enum class SceneContactEventType2D
{
    Begin,
    End
};

struct SceneRaycastHit2D
{
    EntityId entity = 0;
    Vector2 point{}; // world pixels
    Vector2 normal{};
    float fraction = 0.0F;
};

struct SceneContactEvent2D
{
    SceneContactEventType2D type = SceneContactEventType2D::Begin;
    EntityId entityA = 0;
    EntityId entityB = 0;
};

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

    void Start(AudioEngine* audioEngine = nullptr, const AssetRegistry* assets = nullptr) noexcept;
    void Update(float deltaSeconds) noexcept;
    void Stop() noexcept;
    [[nodiscard]] bool IsStarted() const { return m_started; }
    [[nodiscard]] std::size_t GetPhysicsBodyCount() const { return m_physicsBodies.size(); }
    [[nodiscard]] std::size_t GetAudioSourceCount() const { return m_audioSources.size(); }
    [[nodiscard]] bool HasAudioSource(EntityId entity) const
    {
        return m_audioSources.find(entity) != m_audioSources.end();
    }
    [[nodiscard]] bool PlayAudioSource(EntityId entity) noexcept;
    [[nodiscard]] bool StopAudioSource(EntityId entity) noexcept;
    [[nodiscard]] bool IsAudioSourcePlaying(EntityId entity) const noexcept;
    [[nodiscard]] const std::vector<std::string>& GetAudioDiagnostics() const { return m_audioDiagnostics; }
    [[nodiscard]] std::size_t GetPhysicsJointCount() const { return m_physicsWorld ? m_physicsWorld->GetJointCount() : 0; }
    [[nodiscard]] const std::vector<std::string>& GetPhysicsDiagnostics() const { return m_physicsDiagnostics; }
    // Events are collected during the most recent Update and remain available until the next Update.
    [[nodiscard]] const std::vector<SceneContactEvent2D>& GetPhysicsContactEvents() const
    {
        return m_physicsContactEvents;
    }

    [[nodiscard]] std::optional<SceneRaycastHit2D> RaycastClosest(
        Vector2 origin, Vector2 translation, const PhysicsQueryFilter2D& filter = {}) const;
    [[nodiscard]] std::vector<EntityId> OverlapAABB(
        Vector2 lowerBound, Vector2 upperBound, const PhysicsQueryFilter2D& filter = {}) const;

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
    void BuildPhysicsWorld() noexcept;
    void BuildAudioSources(const AssetRegistry* assets) noexcept;
    void DestroyAudioSources() noexcept;
    void DestroyPhysicsWorld() noexcept;
    void StepPhysics(float deltaSeconds) noexcept;

    std::string m_name;
    EntityId m_nextEntityId = 1;
    std::map<EntityId, std::unique_ptr<Entity>> m_entities;
    bool m_started = false;
    std::unique_ptr<PhysicsWorld> m_physicsWorld;
    AudioEngine* m_audioEngine = nullptr;
    std::map<EntityId, AudioSoundHandle> m_audioSources;
    std::vector<std::string> m_audioDiagnostics;
    std::map<EntityId, PhysicsBodyHandle> m_physicsBodies;
    std::vector<std::string> m_physicsDiagnostics;
    std::vector<SceneContactEvent2D> m_physicsContactEvents;
    // Counts overlapping shape-pairs for each normalized Entity pair. Public
    // Begin/End events are emitted only on 0->1 and 1->0 transitions.
    std::map<std::pair<EntityId, EntityId>, std::size_t> m_activeContactCounts;
    float m_physicsAccumulator = 0.0F;
};
} // namespace ember
