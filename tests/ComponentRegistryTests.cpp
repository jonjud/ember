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

    if (failures != 0)
    {
        std::cerr << failures << " component registry test(s) failed.\n";
        return 1;
    }
    std::cout << "All component registry tests passed.\n";
    return 0;
}
