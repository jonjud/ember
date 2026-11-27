#pragma once

#include <memory>

namespace ember
{
class Entity;

// Runtime behavior attached to an Entity. Components are cloned with their
// owning entity and receive lifecycle callbacks from their Scene.
class Component
{
public:
    virtual ~Component() = default;
    [[nodiscard]] virtual std::unique_ptr<Component> Clone() const = 0;

    virtual void OnCreate(Entity& entity) noexcept { (void)entity; }
    virtual void OnUpdate(Entity& entity, float deltaSeconds) noexcept
    {
        (void)entity;
        (void)deltaSeconds;
    }
    virtual void OnDestroy(Entity& entity) noexcept { (void)entity; }
    // Called on components attached to the Entity whose UI Button was clicked.
    virtual void OnUIButtonClicked(Entity& entity) noexcept { (void)entity; }
};
} // namespace ember
