#pragma once

namespace ember
{
struct Vector2
{
    float x = 0.0F;
    float y = 0.0F;

    constexpr Vector2() = default;
    constexpr Vector2(float xValue, float yValue) : x(xValue), y(yValue) {}

    [[nodiscard]] constexpr Vector2 operator+(const Vector2& other) const
    {
        return {x + other.x, y + other.y};
    }

    [[nodiscard]] constexpr Vector2 operator-(const Vector2& other) const
    {
        return {x - other.x, y - other.y};
    }

    [[nodiscard]] constexpr Vector2 operator*(float scalar) const
    {
        return {x * scalar, y * scalar};
    }
};
} // namespace ember
