#include "Engine/Platform/InputMap.hpp"

#include "Engine/Platform/Input.hpp"

#include <algorithm>
#include <utility>
#include <unordered_set>

namespace ember
{
bool InputMap::CreateAction(std::string_view action)
{
    if (action.empty()) return false;
    return m_bindings.try_emplace(std::string(action)).second;
}

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

bool InputMap::RenameAction(std::string_view oldName, std::string_view newName)
{
    if (oldName.empty() || newName.empty() || oldName == newName ||
        m_bindings.contains(std::string(newName)))
    {
        return false;
    }

    const auto oldIt = m_bindings.find(std::string(oldName));
    if (oldIt == m_bindings.end())
    {
        return false;
    }

    auto keys = std::move(oldIt->second);
    m_bindings.erase(oldIt);
    m_bindings.emplace(std::string(newName), std::move(keys));
    return true;
}

void InputMap::Clear()
{
    m_bindings.clear();
    m_actionStates.clear();
}

void InputMap::Update()
{
    std::vector<SDL_Scancode> downKeys;
    for (const auto& [action, keys] : m_bindings)
    {
        (void)action;
        for (const SDL_Scancode key : keys)
        {
            if (Input::IsKeyDown(key)) downKeys.push_back(key);
        }
    }
    Update(downKeys);
}

void InputMap::Update(const std::vector<SDL_Scancode>& downKeys)
{
    const std::unordered_set<SDL_Scancode> pressedKeys(downKeys.begin(), downKeys.end());
    for (const auto& [action, keys] : m_bindings)
    {
        ActionState& state = m_actionStates[action];
        state.previousDown = state.currentDown;
        state.currentDown = std::any_of(keys.begin(), keys.end(),
            [&pressedKeys](SDL_Scancode key) { return pressedKeys.contains(key); });
    }
    for (auto& [action, state] : m_actionStates)
    {
        if (!m_bindings.contains(action))
        {
            state.previousDown = state.currentDown;
            state.currentDown = false;
        }
    }
}

bool InputMap::IsActionDown(std::string_view action) const
{
    const auto state = m_actionStates.find(std::string(action));
    if (state != m_actionStates.end()) return state->second.currentDown;
    for (const SDL_Scancode key : GetBindings(action))
    {
        if (Input::IsKeyDown(key)) return true;
    }
    return false;
}

bool InputMap::WasActionPressed(std::string_view action) const
{
    const auto state = m_actionStates.find(std::string(action));
    return state != m_actionStates.end() && state->second.currentDown &&
        !state->second.previousDown;
}

bool InputMap::WasActionReleased(std::string_view action) const
{
    const auto state = m_actionStates.find(std::string(action));
    return state != m_actionStates.end() && !state->second.currentDown &&
        state->second.previousDown;
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
