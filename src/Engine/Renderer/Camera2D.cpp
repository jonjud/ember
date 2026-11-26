#include "Engine/Renderer/Camera2D.hpp"

#include <algorithm>

namespace ember
{
void Camera2D::SetZoom(float zoom)
{
    m_zoom = std::max(0.01F, zoom);
}

Vector2 Camera2D::WorldToScreen(const Vector2& worldPosition) const
{
    const Vector2 viewportCenter{m_viewportSize.x * 0.5F, m_viewportSize.y * 0.5F};
    return viewportCenter + (worldPosition - m_position) * m_zoom;
}

Vector2 Camera2D::WorldToScreenSize(const Vector2& worldSize) const
{
    return worldSize * m_zoom;
}
} // namespace ember
