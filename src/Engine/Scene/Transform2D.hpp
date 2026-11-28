#pragma once

#include "Engine/Math/Vector2.hpp"

namespace ember
{
// Rotation is stored in radians. Position is parent-local (world-space for root Entities).
struct Transform2D
{
    Vector2 position{};
    Vector2 scale{1.0F, 1.0F};
    float rotation = 0.0F;
};
} // namespace ember
