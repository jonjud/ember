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
    return ok;
}
} // namespace ember
