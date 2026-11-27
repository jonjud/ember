#pragma once

#include "Engine/Math/Vector2.hpp"
#include "Engine/UI/UIComponents2D.hpp"

#include <algorithm>
#include <cmath>

namespace ember
{
struct UIRect2D
{
    Vector2 position{};
    Vector2 size{};
};

struct UICanvasLayout2D
{
    Vector2 origin{};
    float scale = 1.0F;
    Vector2 viewportSize{};
};

[[nodiscard]] inline bool ResolveUICanvasLayout(
    const UICanvas2D& canvas, Vector2 viewportSize, UICanvasLayout2D& result) noexcept
{
    if (!canvas.enabled || !canvas.IsConfigurationValid() ||
        !std::isfinite(viewportSize.x) || !std::isfinite(viewportSize.y) ||
        viewportSize.x <= 0.0F || viewportSize.y <= 0.0F)
        return false;

    const float scaleX = viewportSize.x / canvas.referenceResolution.x;
    const float scaleY = viewportSize.y / canvas.referenceResolution.y;
    const float scale = std::min(scaleX, scaleY);
    if (!std::isfinite(scale) || scale <= 0.0F)
        return false;

    const Vector2 renderedSize = canvas.referenceResolution * scale;
    result.origin = (viewportSize - renderedSize) * 0.5F;
    result.scale = scale;
    result.viewportSize = viewportSize;
    return true;
}

[[nodiscard]] inline bool ResolveUIPanelRect(
    const UIPanel2D& panel, const UICanvasLayout2D& canvas, UIRect2D& result) noexcept
{
    if (!panel.enabled || !panel.IsConfigurationValid() ||
        !std::isfinite(canvas.scale) || canvas.scale <= 0.0F)
        return false;

    result.position = canvas.origin + panel.position * canvas.scale;
    result.size = panel.size * canvas.scale;
    return true;
}
} // namespace ember
