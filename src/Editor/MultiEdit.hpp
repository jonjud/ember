#pragma once

#include "Engine/Scene/ComponentRegistry.hpp"
#include "Engine/Scene/Entity.hpp"

#include <string>
#include <utility>
#include <vector>

namespace ember::editor
{
[[nodiscard]] inline bool HasMixedValues(const std::vector<nlohmann::json>& values)
{
    if (values.size() < 2) return false;
    for (std::size_t index = 1; index < values.size(); ++index)
        if (values[index] != values.front()) return true;
    return false;
}

// Validate against cloned components before touching live instances. If an
// unexpected live setter failure occurs, restore the values already changed.
[[nodiscard]] inline bool ApplyComponentPropertyAtomically(
    const std::vector<Component*>& components,
    const ComponentPropertyInfo& property,
    const nlohmann::json& value)
{
    if (components.empty() || !property.get || !property.set) return false;

    std::vector<std::unique_ptr<Component>> validationCopies;
    std::vector<nlohmann::json> previousValues;
    try
    {
        validationCopies.reserve(components.size());
        previousValues.reserve(components.size());

        for (Component* component : components)
        {
            if (component == nullptr) return false;
            auto copy = component->Clone();
            if (!copy || !property.set(*copy, value)) return false;
            previousValues.push_back(property.get(*component));
            validationCopies.push_back(std::move(copy));
        }
    }
    catch (...)
    {
        // Cloning, getters and validation setters run before live mutation.
        return false;
    }

    std::size_t attempted = 0;
    bool succeeded = true;
    try
    {
        for (std::size_t index = 0; index < components.size(); ++index)
        {
            // Count the current component before calling its setter because it
            // may mutate and then return false or throw.
            attempted = index + 1;
            if (!property.set(*components[index], value))
            {
                succeeded = false;
                break;
            }
        }
    }
    catch (...)
    {
        succeeded = false;
    }
    if (succeeded) return true;

    while (attempted > 0)
    {
        --attempted;
        try
        {
            (void)property.set(*components[attempted], previousValues[attempted]);
        }
        catch (...)
        {
            // Continue restoring earlier components even if one rollback setter
            // itself fails. Built-in registry setters are non-throwing in normal
            // operation; custom descriptors should support restoring old values.
        }
    }
    return false;
}

// Add a registered component to every selected Entity that does not already
// contain it. Factories and capacity reservations run before any live mutation.
[[nodiscard]] inline bool AddComponentToEntities(
    const std::vector<Entity*>& entities,
    const ComponentRegistry& registry,
    const std::string& componentId)
{
    if (entities.empty() || componentId.empty()) return false;

    struct PendingComponent
    {
        Entity* entity = nullptr;
        std::unique_ptr<Component> component;
    };
    std::vector<PendingComponent> pending;
    pending.reserve(entities.size());

    for (Entity* entity : entities)
    {
        if (entity == nullptr || !entity->CanModifyComponents()) return false;
        bool alreadyPresent = false;
        for (const auto& existing : entity->GetComponents())
        {
            const auto* info = registry.Find(*existing);
            if (info != nullptr && info->id == componentId)
            {
                alreadyPresent = true;
                break;
            }
        }
        if (alreadyPresent) continue;
        auto component = registry.Create(componentId);
        if (!component) return false;
        pending.push_back({entity, std::move(component)});
    }

    if (pending.empty()) return false;
    try
    {
        for (auto& item : pending)
            item.entity->GetComponents().reserve(item.entity->GetComponents().size() + 1);
    }
    catch (...) { return false; }

    std::vector<std::pair<Entity*, Component*>> added;
    added.reserve(pending.size());
    try
    {
        for (auto& item : pending)
        {
            Component* raw = item.component.get();
            (void)item.entity->AddComponent(std::move(item.component));
            added.emplace_back(item.entity, raw);
        }
    }
    catch (...)
    {
        for (auto it = added.rbegin(); it != added.rend(); ++it)
            (void)it->first->RemoveComponent(it->second);
        return false;
    }
    return true;
}

[[nodiscard]] inline bool RemoveComponentFromEntities(
    const std::vector<Entity*>& entities,
    const ComponentRegistry& registry,
    const std::string& componentId)
{
    if (entities.empty() || componentId.empty()) return false;

    // Resolve every target and validate editability before removing anything.
    // A running Scene rejects structural component changes, so fail atomically
    // instead of removing from an earlier Entity and failing on a later one.
    std::vector<std::pair<Entity*, Component*>> matches;
    matches.reserve(entities.size());
    for (Entity* entity : entities)
    {
        if (entity == nullptr || !entity->CanModifyComponents()) return false;
        Component* match = nullptr;
        for (const auto& component : entity->GetComponents())
        {
            const auto* info = registry.Find(*component);
            if (info != nullptr && info->id == componentId)
            {
                match = component.get();
                break;
            }
        }
        if (match != nullptr) matches.emplace_back(entity, match);
    }

    if (matches.empty()) return false;
    for (const auto& [entity, component] : matches)
    {
        if (!entity->RemoveComponent(component)) return false;
    }
    return true;
}

} // namespace ember::editor
