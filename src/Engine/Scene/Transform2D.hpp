#pragma once

#include "Engine/Math/Vector2.hpp"

namespace ember
{
struct Transform2D
{
    Vector2 position{};
    Vector2 scale{1.0F, 1.0F};
};
} // namespace ember
