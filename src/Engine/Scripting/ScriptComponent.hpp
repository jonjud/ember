#pragma once

#include "Engine/Scene/Component.hpp"

#include <string>
#include <memory>

namespace ember
{
// Serializable authoring data for a script attached to an Entity.
// An empty asset ID means the component is present but not yet assigned.
class ScriptComponent final : public Component
{
public:
    std::string scriptAssetId;
    bool enabled = true;

    [[nodiscard]] bool IsConfigurationValid() const noexcept
    {
        return scriptAssetId.size() <= 256 &&
            scriptAssetId.find('\0') == std::string::npos;
    }

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<ScriptComponent>(*this);
    }
};
} // namespace ember
