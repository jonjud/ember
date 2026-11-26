#pragma once

#include "Engine/Math/Vector2.hpp"
#include "Engine/Resources/TextureHandle.hpp"

#include <cstdint>

namespace ember
{
// A visual can be a filled rectangle or a renderer-owned texture.
struct RenderComponent2D
{
    Vector2 size{64.0F, 64.0F};
    TextureHandle texture{};
    std::uint8_t red = 255;
    std::uint8_t green = 255;
    std::uint8_t blue = 255;
    std::uint8_t alpha = 255;
    bool visible = false;
};
} // namespace ember
