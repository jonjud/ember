#pragma once

#include "Engine/Math/Vector2.hpp"

#include <cstdint>

namespace ember
{
// Minimal filled-rectangle visual. Texture/sprite rendering can extend this later.
struct RenderComponent2D
{
    Vector2 size{64.0F, 64.0F};
    std::uint8_t red = 255;
    std::uint8_t green = 255;
    std::uint8_t blue = 255;
    std::uint8_t alpha = 255;
    bool visible = false;
};
} // namespace ember
