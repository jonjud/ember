#pragma once

#include "Engine/Math/Vector2.hpp"
#include "Engine/Scene/Component.hpp"

#include <cmath>
#include <memory>

namespace ember
{
class UICanvas2D final : public Component
{
public:
    Vector2 referenceResolution{1280.0F, 720.0F};
    bool enabled = true;

    [[nodiscard]] bool IsConfigurationValid() const noexcept
    {
        return std::isfinite(referenceResolution.x) &&
            std::isfinite(referenceResolution.y) &&
            referenceResolution.x > 0.0F && referenceResolution.y > 0.0F &&
            referenceResolution.x <= 32768.0F && referenceResolution.y <= 32768.0F;
    }

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<UICanvas2D>(*this);
    }
};

class UIPanel2D final : public Component
{
public:
    Vector2 position{0.0F, 0.0F};
    Vector2 size{240.0F, 120.0F};
    float red = 0.12F;
    float green = 0.14F;
    float blue = 0.20F;
    float alpha = 1.0F;
    bool enabled = true;

    [[nodiscard]] bool IsConfigurationValid() const noexcept
    {
        const auto finite = [](float value) { return std::isfinite(value); };
        return finite(position.x) && finite(position.y) &&
            finite(size.x) && finite(size.y) && size.x > 0.0F && size.y > 0.0F &&
            finite(red) && finite(green) && finite(blue) && finite(alpha) &&
            red >= 0.0F && red <= 1.0F && green >= 0.0F && green <= 1.0F &&
            blue >= 0.0F && blue <= 1.0F && alpha >= 0.0F && alpha <= 1.0F;
    }

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<UIPanel2D>(*this);
    }
};
} // namespace ember
