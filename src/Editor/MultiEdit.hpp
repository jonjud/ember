#pragma once

#include "Engine/Scene/ComponentRegistry.hpp"

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

    std::size_t applied = 0;
    for (; applied < components.size(); ++applied)
    {
        if (!property.set(*components[applied], value)) break;
    }
    if (applied == components.size()) return true;

    while (applied > 0)
    {
        --applied;
        (void)property.set(*components[applied], previousValues[applied]);
    }
    return false;
}
} // namespace ember::editor
