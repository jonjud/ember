#include "Engine/Scripting/ScriptComponent.hpp"
#include "Engine/Scripting/ScriptComponentRegistration.hpp"

#include <iostream>
#include <memory>
#include <string>

namespace
{
int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    } else {
        std::cout << "[PASS] " << message << '\n';
    }
}
}

int main()
{
    ember::ScriptComponent component;
    Check(component.IsConfigurationValid(), "Unassigned Script component is a valid editor state");
    component.scriptAssetId = "scripts/player.lua";
    component.enabled = false;
    Check(component.IsConfigurationValid(), "A project-relative script asset ID is valid");

    auto cloneBase = component.Clone();
    auto* clone = dynamic_cast<ember::ScriptComponent*>(cloneBase.get());
    Check(clone != nullptr && clone != &component, "Clone creates an independent Script component");
    Check(clone != nullptr && clone->scriptAssetId == component.scriptAssetId &&
        clone->enabled == component.enabled, "Clone preserves serialized script settings");

    component.scriptAssetId.assign(257, 'x');
    Check(!component.IsConfigurationValid(), "Overlong script asset IDs are rejected");
    component.scriptAssetId = std::string("scripts/invalid") + '\0' + ".lua";
    Check(!component.IsConfigurationValid(), "Script asset IDs containing null bytes are rejected");

    ember::ComponentRegistry registry;
    Check(ember::RegisterScriptComponent(registry), "Script component registers with the generic Component registry");
    component.scriptAssetId = "scripts/player.lua";
    component.enabled = true;
    nlohmann::json data;
    Check(registry.Serialize(component, data), "Script component serializes through the registry");
    auto restoredBase = registry.Create("ember.script", data);
    auto* restored = dynamic_cast<ember::ScriptComponent*>(restoredBase.get());
    Check(restored != nullptr && restored->scriptAssetId == component.scriptAssetId &&
        restored->enabled, "Script component round-trips through registry serialization");

    auto invalid = data;
    invalid["script_asset_id"] = std::string(257, 'x');
    Check(!registry.Create("ember.script", invalid), "Malformed Script component data is rejected");
    invalid = data;
    invalid["enabled"] = "yes";
    Check(!registry.Create("ember.script", invalid), "Wrongly typed Script component fields are rejected");

    return failures == 0 ? 0 : 1;
}
