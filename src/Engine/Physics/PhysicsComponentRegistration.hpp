#pragma once

#include "Engine/Physics/PhysicsComponents2D.hpp"
#include "Engine/Scene/ComponentRegistry.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace ember
{
namespace detail
{
inline bool ReadPhysicsFloat(const nlohmann::json& data, const char* key, float& output)
{
    if (!data.contains(key) || !data.at(key).is_number()) return false;
    const double value = data.at(key).get<double>();
    if (!std::isfinite(value) ||
        std::abs(value) > static_cast<double>(std::numeric_limits<float>::max()))
        return false;
    output = static_cast<float>(value);
    return true;
}

inline bool ReadPhysicsVector2(const nlohmann::json& data, const char* key, Vector2& output)
{
    if (!data.contains(key) || !data.at(key).is_array() || data.at(key).size() != 2)
        return false;
    const auto& value = data.at(key);
    if (!value[0].is_number() || !value[1].is_number()) return false;
    const double x = value[0].get<double>();
    const double y = value[1].get<double>();
    if (!std::isfinite(x) || !std::isfinite(y) ||
        std::abs(x) > std::numeric_limits<float>::max() ||
        std::abs(y) > std::numeric_limits<float>::max()) return false;
    output = {static_cast<float>(x), static_cast<float>(y)};
    return true;
}

inline bool ReadPhysicsBool(const nlohmann::json& data, const char* key, bool& output)
{
    if (!data.contains(key) || !data.at(key).is_boolean()) return false;
    output = data.at(key).get<bool>();
    return true;
}

inline bool ValidMaterial(float density, float friction, float restitution)
{
    return density >= 0.0F && friction >= 0.0F && restitution >= 0.0F;
}
}

inline bool RegisterPhysicsComponents(ComponentRegistry& registry)
{
    bool ok = true;
    ok = registry.Register<ColliderSet2D>("ember.collider_set_2d", "Collider Set 2D",
        [](const Component& base) {
            const auto& set = static_cast<const ColliderSet2D&>(base);
            nlohmann::json entries = nlohmann::json::array();
            for (const auto& c : set.colliders) {
                entries.push_back({{"id", c.id}, {"shape", static_cast<int>(c.shape)},
                    {"offset", {c.offset.x, c.offset.y}}, {"half_extents", {c.halfExtents.x, c.halfExtents.y}},
                    {"radius", c.radius}, {"rotation", c.rotation}, {"density", c.density},
                    {"friction", c.friction}, {"restitution", c.restitution}, {"sensor", c.isSensor},
                    {"category_bits", c.categoryBits}, {"mask_bits", c.maskBits}});
            }
            return nlohmann::json{{"colliders", std::move(entries)}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() || !data.contains("colliders") || !data.at("colliders").is_array()) return false;
            auto& set = static_cast<ColliderSet2D&>(base);
            set.colliders.clear();
            for (const auto& item : data.at("colliders")) {
                if (!item.is_object() || !item.contains("id") || !item.at("id").is_number_unsigned() ||
                    !item.contains("shape") || !item.at("shape").is_number_integer()) return false;
                const auto id = item.at("id").get<ColliderId>();
                const int shape = item.at("shape").get<int>();
                if (id == 0 || (shape != 0 && shape != 1) || !ReserveColliderId(id)) return false;
                Collider2D c(static_cast<ColliderShape2D>(shape), id);
                if (!detail::ReadPhysicsVector2(item, "offset", c.offset) ||
                    !detail::ReadPhysicsVector2(item, "half_extents", c.halfExtents) ||
                    !detail::ReadPhysicsFloat(item, "radius", c.radius) ||
                    !detail::ReadPhysicsFloat(item, "rotation", c.rotation) ||
                    !detail::ReadPhysicsFloat(item, "density", c.density) ||
                    !detail::ReadPhysicsFloat(item, "friction", c.friction) ||
                    !detail::ReadPhysicsFloat(item, "restitution", c.restitution) ||
                    !detail::ReadPhysicsBool(item, "sensor", c.isSensor) ||
                    !item.contains("category_bits") || !item.at("category_bits").is_number_unsigned() ||
                    !item.contains("mask_bits") || !item.at("mask_bits").is_number_unsigned()) return false;
                const auto category = item.at("category_bits").get<std::uint64_t>();
                const auto mask = item.at("mask_bits").get<std::uint64_t>();
                if (category > 65535 || mask > 65535 ||
                    (c.shape == ColliderShape2D::Box && (c.halfExtents.x <= 0 || c.halfExtents.y <= 0)) ||
                    (c.shape == ColliderShape2D::Circle && c.radius <= 0) ||
                    !detail::ValidMaterial(c.density, c.friction, c.restitution)) return false;
                c.categoryBits = static_cast<std::uint16_t>(category);
                c.maskBits = static_cast<std::uint16_t>(mask);
                set.colliders.push_back(std::move(c));
            }
            return set.HasValidColliderIds();
        }) && ok;
    ok = registry.Register<PhysicsBody2D>("ember.physics_body_2d", "Physics Body 2D",
        [](const Component& base) {
            const auto& c = static_cast<const PhysicsBody2D&>(base);
            return nlohmann::json{{"type", static_cast<int>(c.type)},
                {"gravity_scale", c.gravityScale}, {"linear_damping", c.linearDamping},
                {"angular_damping", c.angularDamping}, {"enabled", c.enabled}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() || !data.contains("type") || !data.at("type").is_number_integer())
                return false;
            const int type = data.at("type").get<int>();
            auto& c = static_cast<PhysicsBody2D&>(base);
            if (type < 0 || type > 2) return false;
            c.type = static_cast<PhysicsBodyType2D>(type);
            return detail::ReadPhysicsFloat(data, "gravity_scale", c.gravityScale) &&
                detail::ReadPhysicsFloat(data, "linear_damping", c.linearDamping) &&
                detail::ReadPhysicsFloat(data, "angular_damping", c.angularDamping) &&
                detail::ReadPhysicsBool(data, "enabled", c.enabled) &&
                c.gravityScale >= 0.0F && c.linearDamping >= 0.0F && c.angularDamping >= 0.0F;
        }) && ok;
    ok = registry.Register<BoxCollider2D>("ember.box_collider_2d", "Box Collider 2D",
        [](const Component& base) {
            const auto& c = static_cast<const BoxCollider2D&>(base);
            return nlohmann::json{{"half_extents", {c.halfExtents.x, c.halfExtents.y}},
                {"offset", {c.offset.x, c.offset.y}}, {"rotation", c.rotation},
                {"density", c.density}, {"friction", c.friction},
                {"restitution", c.restitution}, {"sensor", c.isSensor}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object()) return false;
            auto& c = static_cast<BoxCollider2D&>(base);
            return detail::ReadPhysicsVector2(data, "half_extents", c.halfExtents) &&
                detail::ReadPhysicsVector2(data, "offset", c.offset) &&
                detail::ReadPhysicsFloat(data, "rotation", c.rotation) &&
                detail::ReadPhysicsFloat(data, "density", c.density) &&
                detail::ReadPhysicsFloat(data, "friction", c.friction) &&
                detail::ReadPhysicsFloat(data, "restitution", c.restitution) &&
                detail::ReadPhysicsBool(data, "sensor", c.isSensor) &&
                c.halfExtents.x > 0.0F && c.halfExtents.y > 0.0F &&
                detail::ValidMaterial(c.density, c.friction, c.restitution);
        }) && ok;
    ok = registry.Register<CircleCollider2D>("ember.circle_collider_2d", "Circle Collider 2D",
        [](const Component& base) {
            const auto& c = static_cast<const CircleCollider2D&>(base);
            return nlohmann::json{{"radius", c.radius}, {"offset", {c.offset.x, c.offset.y}},
                {"density", c.density}, {"friction", c.friction},
                {"restitution", c.restitution}, {"sensor", c.isSensor}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object()) return false;
            auto& c = static_cast<CircleCollider2D&>(base);
            return detail::ReadPhysicsFloat(data, "radius", c.radius) &&
                detail::ReadPhysicsVector2(data, "offset", c.offset) &&
                detail::ReadPhysicsFloat(data, "density", c.density) &&
                detail::ReadPhysicsFloat(data, "friction", c.friction) &&
                detail::ReadPhysicsFloat(data, "restitution", c.restitution) &&
                detail::ReadPhysicsBool(data, "sensor", c.isSensor) &&
                c.radius > 0.0F && detail::ValidMaterial(c.density, c.friction, c.restitution);
        }) && ok;


    ok = registry.Register<RevoluteJoint2D>("ember.revolute_joint_2d", "Revolute Joint 2D",
        [](const Component& base) {
            const auto& c = static_cast<const RevoluteJoint2D&>(base);
            return nlohmann::json{{"connected_entity", c.connectedEntity},
                {"local_anchor_a", {c.localAnchorA.x, c.localAnchorA.y}},
                {"local_anchor_b", {c.localAnchorB.x, c.localAnchorB.y}},
                {"enable_limit", c.enableLimit}, {"lower_angle", c.lowerAngle},
                {"upper_angle", c.upperAngle}, {"enable_motor", c.enableMotor},
                {"motor_speed", c.motorSpeed}, {"max_motor_torque", c.maxMotorTorque},
                {"collide_connected", c.collideConnected}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() || !data.contains("connected_entity") ||
                !data.at("connected_entity").is_number_unsigned()) return false;
            const auto target = data.at("connected_entity").get<EntityId>();
            auto& c = static_cast<RevoluteJoint2D&>(base);
            c.connectedEntity = target;
            return detail::ReadPhysicsVector2(data, "local_anchor_a", c.localAnchorA) &&
                detail::ReadPhysicsVector2(data, "local_anchor_b", c.localAnchorB) &&
                detail::ReadPhysicsBool(data, "enable_limit", c.enableLimit) &&
                detail::ReadPhysicsFloat(data, "lower_angle", c.lowerAngle) &&
                detail::ReadPhysicsFloat(data, "upper_angle", c.upperAngle) &&
                detail::ReadPhysicsBool(data, "enable_motor", c.enableMotor) &&
                detail::ReadPhysicsFloat(data, "motor_speed", c.motorSpeed) &&
                detail::ReadPhysicsFloat(data, "max_motor_torque", c.maxMotorTorque) &&
                detail::ReadPhysicsBool(data, "collide_connected", c.collideConnected) &&
                c.lowerAngle <= c.upperAngle && c.maxMotorTorque >= 0.0F;
        }) && ok;
    ok = registry.Register<DistanceJoint2D>("ember.distance_joint_2d", "Distance Joint 2D",
        [](const Component& base) {
            const auto& c = static_cast<const DistanceJoint2D&>(base);
            return nlohmann::json{{"connected_entity", c.connectedEntity},
                {"local_anchor_a", {c.localAnchorA.x, c.localAnchorA.y}},
                {"local_anchor_b", {c.localAnchorB.x, c.localAnchorB.y}},
                {"length", c.length}, {"collide_connected", c.collideConnected}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() || !data.contains("connected_entity") ||
                !data.at("connected_entity").is_number_unsigned()) return false;
            const auto target = data.at("connected_entity").get<EntityId>();
            if (target == 0) return false;
            auto& c = static_cast<DistanceJoint2D&>(base);
            c.connectedEntity = target;
            return detail::ReadPhysicsVector2(data, "local_anchor_a", c.localAnchorA) &&
                detail::ReadPhysicsVector2(data, "local_anchor_b", c.localAnchorB) &&
                detail::ReadPhysicsFloat(data, "length", c.length) &&
                detail::ReadPhysicsBool(data, "collide_connected", c.collideConnected) &&
                c.length > 0.0F;
        }) && ok;

    ok = registry.RegisterEnumProperty<PhysicsBody2D>("ember.physics_body_2d", "type", "Body Type",
        &PhysicsBody2D::type, {{PhysicsBodyType2D::Static, "Static"},
            {PhysicsBodyType2D::Kinematic, "Kinematic"}, {PhysicsBodyType2D::Dynamic, "Dynamic"}}) && ok;
    ok = registry.RegisterProperty<PhysicsBody2D>("ember.physics_body_2d", "gravity_scale",
        "Gravity Scale", &PhysicsBody2D::gravityScale) && ok;
    ok = registry.RegisterProperty<PhysicsBody2D>("ember.physics_body_2d", "linear_damping",
        "Linear Damping", &PhysicsBody2D::linearDamping) && ok;
    ok = registry.RegisterProperty<PhysicsBody2D>("ember.physics_body_2d", "angular_damping",
        "Angular Damping", &PhysicsBody2D::angularDamping) && ok;
    ok = registry.RegisterProperty<PhysicsBody2D>("ember.physics_body_2d", "enabled",
        "Enabled", &PhysicsBody2D::enabled) && ok;

    ok = registry.RegisterVector2Property<BoxCollider2D>("ember.box_collider_2d", "half_extents",
        "Half Extents", &BoxCollider2D::halfExtents) && ok;
    ok = registry.RegisterVector2Property<BoxCollider2D>("ember.box_collider_2d", "offset",
        "Offset", &BoxCollider2D::offset) && ok;
    ok = registry.RegisterProperty<BoxCollider2D>("ember.box_collider_2d", "rotation",
        "Rotation (rad)", &BoxCollider2D::rotation) && ok;
    ok = registry.RegisterProperty<BoxCollider2D>("ember.box_collider_2d", "density",
        "Density", &BoxCollider2D::density) && ok;
    ok = registry.RegisterProperty<BoxCollider2D>("ember.box_collider_2d", "friction",
        "Friction", &BoxCollider2D::friction) && ok;
    ok = registry.RegisterProperty<BoxCollider2D>("ember.box_collider_2d", "restitution",
        "Restitution", &BoxCollider2D::restitution) && ok;
    ok = registry.RegisterProperty<BoxCollider2D>("ember.box_collider_2d", "is_sensor",
        "Is Sensor", &BoxCollider2D::isSensor) && ok;

    ok = registry.RegisterProperty<CircleCollider2D>("ember.circle_collider_2d", "radius",
        "Radius", &CircleCollider2D::radius) && ok;
    ok = registry.RegisterVector2Property<CircleCollider2D>("ember.circle_collider_2d", "offset",
        "Offset", &CircleCollider2D::offset) && ok;
    ok = registry.RegisterProperty<CircleCollider2D>("ember.circle_collider_2d", "density",
        "Density", &CircleCollider2D::density) && ok;
    ok = registry.RegisterProperty<CircleCollider2D>("ember.circle_collider_2d", "friction",
        "Friction", &CircleCollider2D::friction) && ok;
    ok = registry.RegisterProperty<CircleCollider2D>("ember.circle_collider_2d", "restitution",
        "Restitution", &CircleCollider2D::restitution) && ok;
    ok = registry.RegisterProperty<CircleCollider2D>("ember.circle_collider_2d", "is_sensor",
        "Is Sensor", &CircleCollider2D::isSensor) && ok;


    ok = registry.RegisterEntityReferenceProperty<RevoluteJoint2D>("ember.revolute_joint_2d",
        "connected_entity", "Connected Entity", &RevoluteJoint2D::connectedEntity) && ok;
    ok = registry.RegisterEntityReferenceProperty<DistanceJoint2D>("ember.distance_joint_2d",
        "connected_entity", "Connected Entity", &DistanceJoint2D::connectedEntity) && ok;

    ok = registry.RegisterVector2Property<RevoluteJoint2D>("ember.revolute_joint_2d",
        "local_anchor_a", "Local Anchor A", &RevoluteJoint2D::localAnchorA) && ok;
    ok = registry.RegisterVector2Property<RevoluteJoint2D>("ember.revolute_joint_2d",
        "local_anchor_b", "Local Anchor B", &RevoluteJoint2D::localAnchorB) && ok;
    ok = registry.RegisterProperty<RevoluteJoint2D>("ember.revolute_joint_2d",
        "enable_limit", "Enable Limit", &RevoluteJoint2D::enableLimit) && ok;
    ok = registry.RegisterProperty<RevoluteJoint2D>("ember.revolute_joint_2d",
        "lower_angle", "Lower Angle (rad)", &RevoluteJoint2D::lowerAngle) && ok;
    ok = registry.RegisterProperty<RevoluteJoint2D>("ember.revolute_joint_2d",
        "upper_angle", "Upper Angle (rad)", &RevoluteJoint2D::upperAngle) && ok;
    ok = registry.RegisterProperty<RevoluteJoint2D>("ember.revolute_joint_2d",
        "enable_motor", "Enable Motor", &RevoluteJoint2D::enableMotor) && ok;
    ok = registry.RegisterProperty<RevoluteJoint2D>("ember.revolute_joint_2d",
        "motor_speed", "Motor Speed (rad/s)", &RevoluteJoint2D::motorSpeed) && ok;
    ok = registry.RegisterProperty<RevoluteJoint2D>("ember.revolute_joint_2d",
        "max_motor_torque", "Max Motor Torque", &RevoluteJoint2D::maxMotorTorque) && ok;
    ok = registry.RegisterProperty<RevoluteJoint2D>("ember.revolute_joint_2d",
        "collide_connected", "Collide Connected", &RevoluteJoint2D::collideConnected) && ok;

    ok = registry.RegisterVector2Property<DistanceJoint2D>("ember.distance_joint_2d",
        "local_anchor_a", "Local Anchor A", &DistanceJoint2D::localAnchorA) && ok;
    ok = registry.RegisterVector2Property<DistanceJoint2D>("ember.distance_joint_2d",
        "local_anchor_b", "Local Anchor B", &DistanceJoint2D::localAnchorB) && ok;
    ok = registry.RegisterProperty<DistanceJoint2D>("ember.distance_joint_2d",
        "length", "Length (pixels)", &DistanceJoint2D::length) && ok;
    ok = registry.RegisterProperty<DistanceJoint2D>("ember.distance_joint_2d",
        "collide_connected", "Collide Connected", &DistanceJoint2D::collideConnected) && ok;

    return ok;
}
} // namespace ember
