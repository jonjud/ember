#include "Engine/Renderer/Camera2D.hpp"

#include <algorithm>
#include <cmath>

namespace ember
{
void Camera2D::SetZoom(float zoom)
{
    m_zoom = std::max(0.01F, zoom);
}

Vector2 Camera2D::WorldToScreen(const Vector2& worldPosition) const
{
    const Vector2 viewportCenter{m_viewportSize.x * 0.5F, m_viewportSize.y * 0.5F};
    const Vector2 offset = worldPosition - m_position;

    // Inverse camera rotation: the world moves opposite to the camera.
    const float cosine = std::cos(m_rotation);
    const float sine = std::sin(m_rotation);
    const Vector2 rotatedOffset{
        offset.x * cosine + offset.y * sine,
        -offset.x * sine + offset.y * cosine
    };

    return viewportCenter + rotatedOffset * m_zoom;
}

Vector2 Camera2D::WorldToScreenSize(const Vector2& worldSize) const
{
    return worldSize * m_zoom;
}
} // namespace ember
