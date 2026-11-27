#pragma once

#include <SDL3/SDL_scancode.h>

namespace ember
{
class Input
{
public:
    [[nodiscard]] static bool IsKeyDown(SDL_Scancode key);
};
} // namespace ember
