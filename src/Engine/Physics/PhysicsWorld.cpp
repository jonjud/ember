#include "Engine/Physics/PhysicsWorld.hpp"

#include <box2d/box2d.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace ember
{
namespace
{
bool IsFinite(Vector2 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool IsValidMaterial(float density, float friction, float restitution)
{
    return std::isfinite(density) && density >= 0.0F &&
           std::isfinite(friction) && friction >= 0.0F &&
           std::isfinite(restitution) && restitution >= 0.0F;
}

b2BodyType ToBox2DBodyType(PhysicsBodyType2D type)
{
    switch (type)
    {
    case PhysicsBodyType2D::Static: return b2_staticBody;
    case PhysicsBodyType2D::Kinematic: return b2_kinematicBody;
    case PhysicsBodyType2D::Dynamic: return b2_dynamicBody;
    }
    throw std::invalid_argument("Invalid PhysicsBodyType2D value.");
}
} // namespace

struct PhysicsWorld::Impl
{
    explicit Impl(const PhysicsWorldDefinition& definition)
    {
        b2WorldDef worldDefinition = b2DefaultWorldDef();
        worldDefinition.gravity = {definition.gravity.x, definition.gravity.y};
        worldId = b2CreateWorld(&worldDefinition);
    }

    ~Impl()
    {
        if (b2World_IsValid(worldId)) b2DestroyWorld(worldId);
    }

    b2WorldId worldId = b2_nullWorldId;
    struct BodyMetadata
    {
        PhysicsBodyHandle handle{};
    };

    std::unordered_map<std::uint64_t, b2BodyId> bodies;
    std::unordered_map<std::uint64_t, std::unique_ptr<BodyMetadata>> bodyMetadata;
    std::vector<PhysicsContactEvent2D> contactEvents;
    std::uint64_t nextHandle = 1;
};

PhysicsWorld::PhysicsWorld(const PhysicsWorldDefinition& definition)
{
    if (!IsFinite(definition.gravity))
        throw std::invalid_argument("PhysicsWorld gravity must be finite.");
    m_impl = std::make_unique<Impl>(definition);
    if (!b2World_IsValid(m_impl->worldId))
        throw std::runtime_error("Box2D failed to create a PhysicsWorld.");
}

PhysicsWorld::~PhysicsWorld() = default;
PhysicsWorld::PhysicsWorld(PhysicsWorld&&) noexcept = default;
PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&&) noexcept = default;

bool PhysicsWorld::IsValid() const
{
    return m_impl && b2World_IsValid(m_impl->worldId);
}

bool PhysicsWorld::SetGravity(Vector2 gravity)
{
    if (!IsValid() || !IsFinite(gravity)) return false;
    b2World_SetGravity(m_impl->worldId, {gravity.x, gravity.y});
    return true;
}

PhysicsBodyHandle PhysicsWorld::CreateBody(const PhysicsBodyDefinition2D& definition)
{
    if (!IsValid()) return {};
    if (!IsFinite(definition.position) || !IsFinite(definition.linearVelocity) ||
        !std::isfinite(definition.rotation) || !std::isfinite(definition.angularVelocity) ||
        !std::isfinite(definition.gravityScale) || !std::isfinite(definition.linearDamping) ||
        !std::isfinite(definition.angularDamping) || definition.gravityScale < 0.0F ||
        definition.linearDamping < 0.0F || definition.angularDamping < 0.0F)
        throw std::invalid_argument("Physics body definition contains invalid values.");

    if (m_impl->nextHandle == 0 || m_impl->nextHandle == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Physics body handle space has been exhausted.");

    b2BodyDef bodyDefinition = b2DefaultBodyDef();
    bodyDefinition.type = ToBox2DBodyType(definition.type);
    bodyDefinition.position = {definition.position.x, definition.position.y};
    bodyDefinition.rotation = b2MakeRot(definition.rotation);
    bodyDefinition.linearVelocity = {definition.linearVelocity.x, definition.linearVelocity.y};
    bodyDefinition.angularVelocity = definition.angularVelocity;
    bodyDefinition.gravityScale = definition.gravityScale;
    bodyDefinition.linearDamping = definition.linearDamping;
    bodyDefinition.angularDamping = definition.angularDamping;

    const b2BodyId bodyId = b2CreateBody(m_impl->worldId, &bodyDefinition);
    if (!b2Body_IsValid(bodyId)) return {};

    const std::uint64_t handle = m_impl->nextHandle++;
    auto metadata = std::make_unique<Impl::BodyMetadata>();
    metadata->handle = {handle};
    auto metadataIt = m_impl->bodyMetadata.emplace(handle, std::move(metadata)).first;
    try
    {
        m_impl->bodies.emplace(handle, bodyId);
    }
    catch (...)
    {
        m_impl->bodyMetadata.erase(metadataIt);
        b2DestroyBody(bodyId);
        throw;
    }
    b2Body_SetUserData(bodyId, metadataIt->second.get());
    return {handle};
}

bool PhysicsWorld::DestroyBody(PhysicsBodyHandle handle)
{
    if (!IsValid() || !handle) return false;
    const auto it = m_impl->bodies.find(handle.value);
    if (it == m_impl->bodies.end() || !b2Body_IsValid(it->second)) return false;
    b2DestroyBody(it->second);
    m_impl->bodies.erase(it);
    m_impl->bodyMetadata.erase(handle.value);
    return true;
}

bool PhysicsWorld::AddBoxCollider(
    PhysicsBodyHandle handle, const PhysicsBoxColliderDefinition2D& definition)
{
    if (!IsValid() || !handle || !IsFinite(definition.halfExtents) || !IsFinite(definition.offset) ||
        definition.halfExtents.x <= 0.0F || definition.halfExtents.y <= 0.0F ||
        !std::isfinite(definition.rotation) ||
        !IsValidMaterial(definition.density, definition.friction, definition.restitution))
        return false;

    const auto it = m_impl->bodies.find(handle.value);
    if (it == m_impl->bodies.end() || !b2Body_IsValid(it->second)) return false;

    b2ShapeDef shapeDefinition = b2DefaultShapeDef();
    shapeDefinition.density = definition.density;
    shapeDefinition.material.friction = definition.friction;
    shapeDefinition.material.restitution = definition.restitution;
    shapeDefinition.isSensor = definition.isSensor;
    shapeDefinition.enableContactEvents = !definition.isSensor;

    const b2Polygon polygon = b2MakeOffsetBox(
        definition.halfExtents.x, definition.halfExtents.y,
        {definition.offset.x, definition.offset.y}, b2MakeRot(definition.rotation));
    return b2Shape_IsValid(b2CreatePolygonShape(it->second, &shapeDefinition, &polygon));
}

bool PhysicsWorld::AddCircleCollider(
    PhysicsBodyHandle handle, const PhysicsCircleColliderDefinition2D& definition)
{
    if (!IsValid() || !handle || !IsFinite(definition.offset) ||
        !std::isfinite(definition.radius) || definition.radius <= 0.0F ||
        !IsValidMaterial(definition.density, definition.friction, definition.restitution))
        return false;

    const auto it = m_impl->bodies.find(handle.value);
    if (it == m_impl->bodies.end() || !b2Body_IsValid(it->second)) return false;

    b2ShapeDef shapeDefinition = b2DefaultShapeDef();
    shapeDefinition.density = definition.density;
    shapeDefinition.material.friction = definition.friction;
    shapeDefinition.material.restitution = definition.restitution;
    shapeDefinition.isSensor = definition.isSensor;
    shapeDefinition.enableContactEvents = !definition.isSensor;

    const b2Circle circle{{definition.offset.x, definition.offset.y}, definition.radius};
    return b2Shape_IsValid(b2CreateCircleShape(it->second, &shapeDefinition, &circle));
}

std::optional<PhysicsTransform2D> PhysicsWorld::GetBodyTransform(PhysicsBodyHandle handle) const
{
    if (!IsValid() || !handle) return std::nullopt;
    const auto it = m_impl->bodies.find(handle.value);
    if (it == m_impl->bodies.end() || !b2Body_IsValid(it->second)) return std::nullopt;

    const b2Pos position = b2Body_GetPosition(it->second);
    const b2Rot rotation = b2Body_GetRotation(it->second);
    return PhysicsTransform2D{
        {static_cast<float>(position.x), static_cast<float>(position.y)},
        b2Rot_GetAngle(rotation)
    };
}

bool PhysicsWorld::SetBodyTransform(PhysicsBodyHandle handle, const PhysicsTransform2D& transform)
{
    if (!IsValid() || !handle || !IsFinite(transform.position) ||
        !std::isfinite(transform.rotation))
        return false;

    const auto it = m_impl->bodies.find(handle.value);
    if (it == m_impl->bodies.end() || !b2Body_IsValid(it->second)) return false;
    b2Body_SetTransform(
        it->second,
        {transform.position.x, transform.position.y},
        b2MakeRot(transform.rotation));
    return true;
}

bool PhysicsWorld::Step(float timeStep, int subStepCount)
{
    if (!IsValid() || !std::isfinite(timeStep) || timeStep <= 0.0F || subStepCount < 1)
        return false;
    b2World_Step(m_impl->worldId, timeStep, subStepCount);
    const b2ContactEvents events = b2World_GetContactEvents(m_impl->worldId);
    const auto appendEvent = [this](b2ShapeId shapeA, b2ShapeId shapeB,
                                    PhysicsContactEventType2D type)
    {
        // End events may refer to destroyed shapes. Runtime Scene structure is locked,
        // but still validate IDs before following Box2D handles.
        if (!b2Shape_IsValid(shapeA) || !b2Shape_IsValid(shapeB)) return;
        const b2BodyId bodyA = b2Shape_GetBody(shapeA);
        const b2BodyId bodyB = b2Shape_GetBody(shapeB);
        if (!b2Body_IsValid(bodyA) || !b2Body_IsValid(bodyB)) return;
        const auto* metadataA = static_cast<const Impl::BodyMetadata*>(b2Body_GetUserData(bodyA));
        const auto* metadataB = static_cast<const Impl::BodyMetadata*>(b2Body_GetUserData(bodyB));
        if (metadataA == nullptr || metadataB == nullptr) return;
        m_impl->contactEvents.push_back({type, metadataA->handle, metadataB->handle});
    };

    for (int index = 0; index < events.beginCount; ++index)
        appendEvent(events.beginEvents[index].shapeIdA, events.beginEvents[index].shapeIdB,
            PhysicsContactEventType2D::Begin);
    for (int index = 0; index < events.endCount; ++index)
        appendEvent(events.endEvents[index].shapeIdA, events.endEvents[index].shapeIdB,
            PhysicsContactEventType2D::End);
    return true;
}

std::vector<PhysicsContactEvent2D> PhysicsWorld::ConsumeContactEvents()
{
    if (!m_impl) return {};
    std::vector<PhysicsContactEvent2D> events = std::move(m_impl->contactEvents);
    m_impl->contactEvents.clear();
    return events;
}

std::size_t PhysicsWorld::GetBodyCount() const
{
    return m_impl ? m_impl->bodies.size() : 0;
}
} // namespace ember
