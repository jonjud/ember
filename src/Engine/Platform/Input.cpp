#include "Engine/Platform/Input.hpp"

#include <SDL3/SDL.h>

namespace ember
{
bool Input::IsKeyDown(SDL_Scancode key)
{
    const bool* keyboardState = SDL_GetKeyboardState(nullptr);
    return keyboardState != nullptr && keyboardState[key];
}
} // namespace ember
