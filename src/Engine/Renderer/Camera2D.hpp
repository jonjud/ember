#pragma once

#include "Engine/Math/Vector2.hpp"

namespace ember
{
class Camera2D
{
public:
    void SetPosition(const Vector2& position) { m_position = position; }
    void SetZoom(float zoom);
    void SetRotation(float radians) { m_rotation = radians; }

    void SetViewportSize(float width, float height)
    {
        m_viewportSize = {width, height};
    }

    [[nodiscard]] const Vector2& GetPosition() const { return m_position; }
    [[nodiscard]] float GetZoom() const { return m_zoom; }
    [[nodiscard]] float GetRotation() const { return m_rotation; }
    [[nodiscard]] const Vector2& GetViewportSize() const { return m_viewportSize; }

    [[nodiscard]] Vector2 WorldToScreen(const Vector2& worldPosition) const;
    [[nodiscard]] Vector2 WorldToScreenSize(const Vector2& worldSize) const;

private:
    Vector2 m_position{};
    Vector2 m_viewportSize{};
    float m_zoom = 1.0F;
    float m_rotation = 0.0F;
};
} // namespace ember
