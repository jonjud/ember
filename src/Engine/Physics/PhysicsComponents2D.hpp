#pragma once

#include "Engine/Physics/PhysicsWorld.hpp"
#include "Engine/Scene/Component.hpp"

#include <atomic>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace ember
{
// Serializable configuration components. Runtime PhysicsBodyHandle values are
// deliberately owned by Scene/PhysicsWorld and never stored in these objects.
using ColliderId = std::uint64_t;

enum class ColliderShape2D
{
    Box,
    Circle
};

// IDs identify authored collider entries, not backend shapes. Copies preserve
// IDs so cloning a Scene does not silently change collider identity.
[[nodiscard]] inline ColliderId GenerateColliderId() noexcept
{
    static std::atomic<ColliderId> nextId{1};
    ColliderId current = nextId.load(std::memory_order_relaxed);
    while (current != 0)
    {
        const ColliderId next = current == std::numeric_limits<ColliderId>::max()
            ? 0 : current + 1;
        if (nextId.compare_exchange_weak(current, next,
            std::memory_order_relaxed, std::memory_order_relaxed))
            return current;
    }
    return 0; // Exhausted: never wrap and reuse an earlier ID.
}

struct Collider2D
{
    ColliderId id = 0;
    ColliderShape2D shape = ColliderShape2D::Box;
    Vector2 offset{};
    Vector2 halfExtents{0.5F, 0.5F};
    float radius = 0.5F;
    float rotation = 0.0F;
    float density = 1.0F;
    float friction = 0.3F;
    float restitution = 0.0F;
    bool isSensor = false;
    std::uint16_t categoryBits = 0x0001;
    std::uint16_t maskBits = 0xFFFF;

    explicit Collider2D(ColliderShape2D shapeKind = ColliderShape2D::Box)
        : id(GenerateColliderId()), shape(shapeKind)
    {
        if (id == 0)
            throw std::overflow_error("Collider ID space exhausted.");
    }
};

class ColliderSet2D final : public Component
{
public:
    std::vector<Collider2D> colliders;

    [[nodiscard]] Collider2D& AddCollider(ColliderShape2D shape)
    {
        colliders.emplace_back(shape);
        return colliders.back();
    }

    [[nodiscard]] Collider2D& AddBoxCollider() { return AddCollider(ColliderShape2D::Box); }
    [[nodiscard]] Collider2D& AddCircleCollider() { return AddCollider(ColliderShape2D::Circle); }

    [[nodiscard]] bool RemoveCollider(ColliderId id)
    {
        const auto it = std::find_if(colliders.begin(), colliders.end(),
            [id](const Collider2D& collider) { return collider.id == id; });
        if (it == colliders.end()) return false;
        colliders.erase(it);
        return true;
    }

    [[nodiscard]] Collider2D* FindCollider(ColliderId id)
    {
        const auto it = std::find_if(colliders.begin(), colliders.end(),
            [id](const Collider2D& collider) { return collider.id == id; });
        return it == colliders.end() ? nullptr : &*it;
    }

    [[nodiscard]] const Collider2D* FindCollider(ColliderId id) const
    {
        const auto it = std::find_if(colliders.begin(), colliders.end(),
            [id](const Collider2D& collider) { return collider.id == id; });
        return it == colliders.end() ? nullptr : &*it;
    }

    [[nodiscard]] bool HasValidColliderIds() const
    {
        for (std::size_t i = 0; i < colliders.size(); ++i)
        {
            if (colliders[i].id == 0) return false;
            for (std::size_t j = i + 1; j < colliders.size(); ++j)
                if (colliders[i].id == colliders[j].id) return false;
        }
        return true;
    }

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<ColliderSet2D>(*this);
    }
};

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
