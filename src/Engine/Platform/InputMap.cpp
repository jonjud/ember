#include "Engine/Platform/InputMap.hpp"

#include "Engine/Platform/Input.hpp"

#include <algorithm>

namespace ember
{
bool InputMap::BindAction(std::string_view action, SDL_Scancode key)
{
    if (action.empty() || key == SDL_SCANCODE_UNKNOWN)
    {
        return false;
    }

    auto& keys = m_bindings[std::string(action)];
    if (std::find(keys.begin(), keys.end(), key) != keys.end())
    {
        return false;
    }

    keys.push_back(key);
    return true;
}

bool InputMap::UnbindAction(std::string_view action, SDL_Scancode key)
{
    const auto actionIt = m_bindings.find(std::string(action));
    if (actionIt == m_bindings.end())
    {
        return false;
    }

    auto& keys = actionIt->second;
    const auto keyIt = std::find(keys.begin(), keys.end(), key);
    if (keyIt == keys.end())
    {
        return false;
    }

    keys.erase(keyIt);
    if (keys.empty())
    {
        m_bindings.erase(actionIt);
    }
    return true;
}

void InputMap::ClearAction(std::string_view action)
{
    m_bindings.erase(std::string(action));
}

void InputMap::Clear()
{
    m_bindings.clear();
}

bool InputMap::IsActionDown(std::string_view action) const
{
    for (const SDL_Scancode key : GetBindings(action))
    {
        if (Input::IsKeyDown(key))
        {
            return true;
        }
    }
    return false;
}

const std::vector<SDL_Scancode>& InputMap::GetBindings(std::string_view action) const
{
    static const std::vector<SDL_Scancode> empty;
    const auto it = m_bindings.find(std::string(action));
    return it != m_bindings.end() ? it->second : empty;
}

InputMap InputMap::CreateDefaultMovementMap()
{
    InputMap map;
    (void)map.BindAction("MoveLeft", SDL_SCANCODE_A);
    (void)map.BindAction("MoveLeft", SDL_SCANCODE_LEFT);
    (void)map.BindAction("MoveRight", SDL_SCANCODE_D);
    (void)map.BindAction("MoveRight", SDL_SCANCODE_RIGHT);
    (void)map.BindAction("MoveUp", SDL_SCANCODE_W);
    (void)map.BindAction("MoveUp", SDL_SCANCODE_UP);
    (void)map.BindAction("MoveDown", SDL_SCANCODE_S);
    (void)map.BindAction("MoveDown", SDL_SCANCODE_DOWN);
    return map;
}
} // namespace ember
