#include "Engine/Scene/ComponentRegistry.hpp"

#include <iostream>
#include <memory>

namespace
{
int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
    else std::cout << "[PASS] " << message << '\n';
}

class MarkerComponent final : public ember::Component
{
public:
    int value = 42;
    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<MarkerComponent>(*this);
    }
};

class OtherComponent final : public ember::Component
{
public:
    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<OtherComponent>(*this);
    }
};
}

int main()
{
    ember::ComponentRegistry registry;
    Check(registry.IsEmpty(), "New registry starts empty");
    Check(!registry.Register<MarkerComponent>("", "Marker"), "Empty stable IDs are rejected");
    Check(!registry.Register<MarkerComponent>("marker", ""), "Empty display names are rejected");
    Check(registry.Register<MarkerComponent>("example.marker", "Marker"),
        "Component type can be registered");
    Check(!registry.Register<MarkerComponent>("example.marker.copy", "Duplicate Type"),
        "A C++ component type cannot be registered twice");
    Check(!registry.Register<OtherComponent>("example.marker", "Duplicate ID"),
        "Stable component IDs must be unique");
    Check(registry.Register<OtherComponent>("example.other", "Other"),
        "A second component type can be registered");

    const auto* markerInfo = registry.Find("example.marker");
    Check(markerInfo != nullptr && markerInfo->displayName == "Marker",
        "Registered metadata can be found by stable ID");

    MarkerComponent source;
    const auto* sourceInfo = registry.Find(source);
    Check(sourceInfo != nullptr && sourceInfo->id == "example.marker",
        "Registered metadata can be found by component instance");

    auto created = registry.Create("example.marker");
    auto* marker = dynamic_cast<MarkerComponent*>(created.get());
    Check(marker != nullptr && marker->value == 42,
        "Factory creates the registered component type");
    Check(registry.Create("missing") == nullptr,
        "Unknown component IDs return no instance");

    const auto types = registry.GetRegisteredTypes();
    Check(types.size() == 2 && types[0].id == "example.marker" &&
          types[1].id == "example.other",
          "Registered types are listed in deterministic ID order");
    Check(registry.Size() == 2, "Registry reports its type count");

    Check(registry.RegisterProperty<MarkerComponent>(
        "example.marker", "value", "Value", &MarkerComponent::value),
        "Registered component properties can be described");
    Check(!registry.RegisterProperty<MarkerComponent>(
        "example.marker", "value", "Duplicate", &MarkerComponent::value),
        "Duplicate property IDs are rejected");
    Check(!registry.RegisterProperty<OtherComponent>(
        "example.marker", "wrong-owner", "Wrong Owner", nullptr),
        "Properties cannot be registered against a different component type");
    const auto* markerProperties = registry.FindProperties(source);
    Check(markerProperties != nullptr && markerProperties->size() == 1 &&
        markerProperties->front().type == ember::ComponentPropertyType::Integer,
        "Property metadata can be queried by component instance");
    if (markerProperties != nullptr && !markerProperties->empty())
    {
        Check(markerProperties->front().get(source).get<int>() == 42,
            "Property descriptor reads the current component value");
        Check(markerProperties->front().set(source, 73) && source.value == 73,
            "Property descriptor updates the component value");
        Check(!markerProperties->front().set(source, "not an integer") && source.value == 73,
            "Property descriptor rejects values with the wrong type");
        Check(!markerProperties->front().set(source, 999999999999LL) && source.value == 73,
            "Integer property rejects values outside the supported range");
    }

    if (failures != 0)
    {
        std::cerr << failures << " component registry test(s) failed.\n";
        return 1;
    }
    std::cout << "All component registry tests passed.\n";
    return 0;
}
