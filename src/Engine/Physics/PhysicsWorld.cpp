#include "Engine/Physics/PhysicsWorld.hpp"

#include <box2d/box2d.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace ember
{
namespace
{
struct PhysicsBodyMetadata { PhysicsBodyHandle handle{}; };
struct RaycastQueryContext { std::optional<PhysicsRaycastHit2D> hit; };
float CollectRaycastHit(b2ShapeId shapeId, b2Pos point, b2Vec2 normal, float fraction, void* context)
{
    auto& query = *static_cast<RaycastQueryContext*>(context);
    if (!b2Shape_IsValid(shapeId)) return -1.0F;
    const b2BodyId body = b2Shape_GetBody(shapeId);
    if (!b2Body_IsValid(body)) return -1.0F;
    const auto* metadata = static_cast<const PhysicsBodyMetadata*>(b2Body_GetUserData(body));
    if (metadata == nullptr || !metadata->handle) return -1.0F;
    query.hit = PhysicsRaycastHit2D{metadata->handle,
        {static_cast<float>(point.x), static_cast<float>(point.y)},
        {normal.x, normal.y}, fraction};
    return fraction;
}
struct OverlapQueryContext { std::vector<PhysicsBodyHandle> bodies; };
bool CollectOverlap(b2ShapeId shapeId, void* context)
{
    auto& query = *static_cast<OverlapQueryContext*>(context);
    if (!b2Shape_IsValid(shapeId)) return true;
    const b2BodyId body = b2Shape_GetBody(shapeId);
    if (!b2Body_IsValid(body)) return true;
    const auto* metadata = static_cast<const PhysicsBodyMetadata*>(b2Body_GetUserData(body));
    if (metadata != nullptr && metadata->handle) query.bodies.push_back(metadata->handle);
    return true;
}
b2QueryFilter ToBox2DQueryFilter(const PhysicsQueryFilter2D& filter)
{
    b2QueryFilter result = b2DefaultQueryFilter();
    result.categoryBits = filter.categoryBits;
    result.maskBits = filter.maskBits;
    return result;
}
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
    struct JointMetadata
    {
        b2JointId id = b2_nullJointId;
        PhysicsBodyHandle bodyA{};
        PhysicsBodyHandle bodyB{};
    };

    std::unordered_map<std::uint64_t, b2BodyId> bodies;
    std::unordered_map<std::uint64_t, std::unique_ptr<PhysicsBodyMetadata>> bodyMetadata;
    std::unordered_map<std::uint64_t, JointMetadata> joints;
    std::vector<PhysicsContactEvent2D> contactEvents;
    std::uint64_t nextHandle = 1;
    std::uint64_t nextJointHandle = 1;
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
    auto metadata = std::make_unique<PhysicsBodyMetadata>();
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
    for (auto joint = m_impl->joints.begin(); joint != m_impl->joints.end();)
    {
        if (joint->second.bodyA == handle || joint->second.bodyB == handle)
        {
            if (b2Joint_IsValid(joint->second.id)) b2DestroyJoint(joint->second.id);
            joint = m_impl->joints.erase(joint);
        }
        else
        {
            ++joint;
        }
    }
    b2DestroyBody(it->second);
    m_impl->bodies.erase(it);
    m_impl->bodyMetadata.erase(handle.value);
    return true;
}

PhysicsJointHandle PhysicsWorld::CreateRevoluteJoint(
    const PhysicsRevoluteJointDefinition2D& definition)
{
    if (!IsValid() || !definition.bodyA || !definition.bodyB ||
        definition.bodyA == definition.bodyB ||
        !IsFinite(definition.localAnchorA) || !IsFinite(definition.localAnchorB) ||
        !std::isfinite(definition.lowerAngle) || !std::isfinite(definition.upperAngle) ||
        !std::isfinite(definition.motorSpeed) || !std::isfinite(definition.maxMotorTorque) ||
        definition.maxMotorTorque < 0.0F ||
        (definition.enableLimit && definition.lowerAngle > definition.upperAngle))
        return {};

    const auto bodyA = m_impl->bodies.find(definition.bodyA.value);
    const auto bodyB = m_impl->bodies.find(definition.bodyB.value);
    if (bodyA == m_impl->bodies.end() || bodyB == m_impl->bodies.end() ||
        !b2Body_IsValid(bodyA->second) || !b2Body_IsValid(bodyB->second) ||
        m_impl->nextJointHandle == 0 ||
        m_impl->nextJointHandle == std::numeric_limits<std::uint64_t>::max())
        return {};

    b2RevoluteJointDef jointDefinition = b2DefaultRevoluteJointDef();
    jointDefinition.base.bodyIdA = bodyA->second;
    jointDefinition.base.bodyIdB = bodyB->second;
    jointDefinition.base.localFrameA = {
        {definition.localAnchorA.x, definition.localAnchorA.y}, b2MakeRot(0.0F)};
    jointDefinition.base.localFrameB = {
        {definition.localAnchorB.x, definition.localAnchorB.y}, b2MakeRot(0.0F)};
    jointDefinition.base.collideConnected = definition.collideConnected;
    jointDefinition.enableLimit = definition.enableLimit;
    jointDefinition.lowerAngle = definition.lowerAngle;
    jointDefinition.upperAngle = definition.upperAngle;
    jointDefinition.enableMotor = definition.enableMotor;
    jointDefinition.motorSpeed = definition.motorSpeed;
    jointDefinition.maxMotorTorque = definition.maxMotorTorque;

    const b2JointId id = b2CreateRevoluteJoint(m_impl->worldId, &jointDefinition);
    if (!b2Joint_IsValid(id)) return {};
    const std::uint64_t handle = m_impl->nextJointHandle++;
    try
    {
        m_impl->joints.emplace(handle, Impl::JointMetadata{id, definition.bodyA, definition.bodyB});
    }
    catch (...)
    {
        b2DestroyJoint(id);
        throw;
    }
    return {handle};
}

PhysicsJointHandle PhysicsWorld::CreateDistanceJoint(
    const PhysicsDistanceJointDefinition2D& definition)
{
    if (!IsValid() || !definition.bodyA || !definition.bodyB ||
        definition.bodyA == definition.bodyB ||
        !IsFinite(definition.localAnchorA) || !IsFinite(definition.localAnchorB) ||
        !std::isfinite(definition.length) || definition.length <= 0.0F)
        return {};

    const auto bodyA = m_impl->bodies.find(definition.bodyA.value);
    const auto bodyB = m_impl->bodies.find(definition.bodyB.value);
    if (bodyA == m_impl->bodies.end() || bodyB == m_impl->bodies.end() ||
        !b2Body_IsValid(bodyA->second) || !b2Body_IsValid(bodyB->second) ||
        m_impl->nextJointHandle == 0 ||
        m_impl->nextJointHandle == std::numeric_limits<std::uint64_t>::max())
        return {};

    b2DistanceJointDef jointDefinition = b2DefaultDistanceJointDef();
    jointDefinition.base.bodyIdA = bodyA->second;
    jointDefinition.base.bodyIdB = bodyB->second;
    jointDefinition.base.localFrameA = {
        {definition.localAnchorA.x, definition.localAnchorA.y}, b2MakeRot(0.0F)};
    jointDefinition.base.localFrameB = {
        {definition.localAnchorB.x, definition.localAnchorB.y}, b2MakeRot(0.0F)};
    jointDefinition.base.collideConnected = definition.collideConnected;
    jointDefinition.length = definition.length;

    const b2JointId id = b2CreateDistanceJoint(m_impl->worldId, &jointDefinition);
    if (!b2Joint_IsValid(id)) return {};
    const std::uint64_t handle = m_impl->nextJointHandle++;
    try
    {
        m_impl->joints.emplace(handle, Impl::JointMetadata{id, definition.bodyA, definition.bodyB});
    }
    catch (...)
    {
        b2DestroyJoint(id);
        throw;
    }
    return {handle};
}

bool PhysicsWorld::DestroyJoint(PhysicsJointHandle handle)
{
    if (!IsValid() || !handle) return false;
    const auto it = m_impl->joints.find(handle.value);
    if (it == m_impl->joints.end()) return false;
    if (b2Joint_IsValid(it->second.id)) b2DestroyJoint(it->second.id);
    m_impl->joints.erase(it);
    return true;
}

std::size_t PhysicsWorld::GetJointCount() const
{
    return m_impl ? m_impl->joints.size() : 0;
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
    shapeDefinition.filter.categoryBits = definition.categoryBits;
    shapeDefinition.filter.maskBits = definition.maskBits;

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
    shapeDefinition.filter.categoryBits = definition.categoryBits;
    shapeDefinition.filter.maskBits = definition.maskBits;

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

    const b2BodyId movedBody = it->second;
    const b2Pos currentPosition = b2Body_GetPosition(movedBody);
    const float currentRotation = b2Rot_GetAngle(b2Body_GetRotation(movedBody));
    const bool hasMoved = currentPosition.x != transform.position.x ||
        currentPosition.y != transform.position.y || currentRotation != transform.rotation;

    // Transform-driven static/kinematic bodies can be teleported by the Scene.
    // Wake dynamic bodies that were touching the moved body so Box2D refreshes
    // their contact state and can emit the corresponding End event.
    std::vector<b2BodyId> bodiesToWake;
    if (hasMoved)
    {
        const int contactCapacity = b2Body_GetContactCapacity(movedBody);
        if (contactCapacity > 0)
        {
            std::vector<b2ContactData> contacts(static_cast<std::size_t>(contactCapacity));
            const int contactCount = b2Body_GetContactData(
                movedBody, contacts.data(), contactCapacity);
            for (int index = 0; index < contactCount; ++index)
            {
                const b2ContactData& contact = contacts[static_cast<std::size_t>(index)];
                if (contact.manifold.pointCount == 0) continue;

                const b2BodyId bodyA = b2Shape_GetBody(contact.shapeIdA);
                const b2BodyId bodyB = b2Shape_GetBody(contact.shapeIdB);
                const bool bodyAIsMoved = bodyA.index1 == movedBody.index1 &&
                    bodyA.world0 == movedBody.world0 && bodyA.generation == movedBody.generation;
                const b2BodyId otherBody = bodyAIsMoved ? bodyB : bodyA;
                if (b2Body_IsValid(otherBody) && b2Body_GetType(otherBody) == b2_dynamicBody)
                    bodiesToWake.push_back(otherBody);
            }
        }
    }

    b2Body_SetTransform(
        movedBody,
        {transform.position.x, transform.position.y},
        b2MakeRot(transform.rotation));
    for (const b2BodyId body : bodiesToWake)
    {
        if (b2Body_IsValid(body)) b2Body_SetAwake(body, true);
    }
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
        const auto* metadataA = static_cast<const PhysicsBodyMetadata*>(b2Body_GetUserData(bodyA));
        const auto* metadataB = static_cast<const PhysicsBodyMetadata*>(b2Body_GetUserData(bodyB));
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

std::optional<PhysicsRaycastHit2D> PhysicsWorld::RaycastClosest(
    Vector2 origin, Vector2 translation, const PhysicsQueryFilter2D& filter) const
{
    if (!IsValid() || !IsFinite(origin) || !IsFinite(translation) ||
        (translation.x == 0.0F && translation.y == 0.0F)) return std::nullopt;
    RaycastQueryContext context;
    (void)b2World_CastRay(m_impl->worldId, {origin.x, origin.y},
        {translation.x, translation.y}, ToBox2DQueryFilter(filter), CollectRaycastHit, &context);
    return context.hit;
}

std::vector<PhysicsBodyHandle> PhysicsWorld::OverlapAABB(
    Vector2 lowerBound, Vector2 upperBound, const PhysicsQueryFilter2D& filter) const
{
    if (!IsValid() || !IsFinite(lowerBound) || !IsFinite(upperBound) ||
        lowerBound.x > upperBound.x || lowerBound.y > upperBound.y) return {};
    const b2AABB bounds{{lowerBound.x, lowerBound.y}, {upperBound.x, upperBound.y}};
    OverlapQueryContext context;
    (void)b2World_OverlapAABB(m_impl->worldId, {0.0F, 0.0F}, bounds,
        ToBox2DQueryFilter(filter), CollectOverlap, &context);
    std::sort(context.bodies.begin(), context.bodies.end(),
        [](PhysicsBodyHandle a, PhysicsBodyHandle b) { return a.value < b.value; });
    context.bodies.erase(std::unique(context.bodies.begin(), context.bodies.end()), context.bodies.end());
    return context.bodies;
}

std::size_t PhysicsWorld::GetBodyCount() const
{
    return m_impl ? m_impl->bodies.size() : 0;
}
} // namespace ember
