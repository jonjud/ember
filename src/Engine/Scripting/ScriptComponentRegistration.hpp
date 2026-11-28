#pragma once

#include "Engine/Scripting/ScriptComponent.hpp"
#include "Engine/Scene/ComponentRegistry.hpp"

namespace ember
{
inline bool RegisterScriptComponent(ComponentRegistry& registry)
{
    bool ok = registry.Register<ScriptComponent>("ember.script", "Script",
        [](const Component& base) {
            const auto& script = static_cast<const ScriptComponent&>(base);
            return nlohmann::json{
                {"script_asset_id", script.scriptAssetId},
                {"enabled", script.enabled}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() ||
                !data.contains("script_asset_id") || !data.at("script_asset_id").is_string() ||
                !data.contains("enabled") || !data.at("enabled").is_boolean())
                return false;
            try {
                auto& script = static_cast<ScriptComponent&>(base);
                script.scriptAssetId = data.at("script_asset_id").get<std::string>();
                script.enabled = data.at("enabled").get<bool>();
                return script.IsConfigurationValid();
            } catch (...) { return false; }
        });
    ok = registry.RegisterProperty<ScriptComponent>(
        "ember.script", "script_asset_id", "Script Asset ID",
        &ScriptComponent::scriptAssetId) && ok;
    ok = registry.RegisterProperty<ScriptComponent>(
        "ember.script", "enabled", "Enabled", &ScriptComponent::enabled) && ok;
    return ok;
}
} // namespace ember
