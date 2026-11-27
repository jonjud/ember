#pragma once

#include "Engine/Scene/Component.hpp"

#include <cmath>
#include <memory>
#include <string>

namespace ember
{
// Serializable authoring configuration for a sound attached to an Entity.
// Runtime AudioSoundHandles are deliberately owned by Scene, never by this component.
class AudioSource2D final : public Component
{
public:
    std::string audioAssetId;
    float volume = 1.0F;
    bool loop = false;
    bool playOnStart = true;
    bool streaming = false;
    bool enabled = true;

    [[nodiscard]] bool IsConfigurationValid() const noexcept
    {
        return audioAssetId.size() <= 256 && std::isfinite(volume) &&
            volume >= 0.0F && volume <= 4.0F;
    }

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<AudioSource2D>(*this);
    }
};
} // namespace ember
