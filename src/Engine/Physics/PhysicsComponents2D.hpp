#pragma once

#include "Engine/Physics/PhysicsWorld.hpp"
#include "Engine/Scene/Component.hpp"

#include <memory>

namespace ember
{
// Serializable configuration components. Runtime PhysicsBodyHandle values are
// deliberately owned by Scene/PhysicsWorld and never stored in these objects.
class PhysicsBody2D final : public Component
{
public:
    PhysicsBodyType2D type = PhysicsBodyType2D::Static;
    float gravityScale = 1.0F;
    float linearDamping = 0.0F;
    float angularDamping = 0.0F;
    bool enabled = true;

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<PhysicsBody2D>(*this);
    }
};

class BoxCollider2D final : public Component
{
public:
    Vector2 halfExtents{0.5F, 0.5F};
    Vector2 offset{};
    float rotation = 0.0F;
    float density = 1.0F;
    float friction = 0.3F;
    float restitution = 0.0F;
    bool isSensor = false;

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<BoxCollider2D>(*this);
    }
};

class CircleCollider2D final : public Component
{
public:
    float radius = 0.5F;
    Vector2 offset{};
    float density = 1.0F;
    float friction = 0.3F;
    float restitution = 0.0F;
    bool isSensor = false;

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<CircleCollider2D>(*this);
    }
};
} // namespace ember
