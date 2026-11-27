#pragma once

#include <SDL3/SDL_scancode.h>

#include <string>
#include <unordered_map>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ember
{
// Maps named game actions to one or more physical keyboard keys.
class InputMap
{
public:
    [[nodiscard]] bool CreateAction(std::string_view action);
    [[nodiscard]] bool BindAction(std::string_view action, SDL_Scancode key);
    [[nodiscard]] bool UnbindAction(std::string_view action, SDL_Scancode key);
    void ClearAction(std::string_view action);
    [[nodiscard]] bool RenameAction(std::string_view oldName, std::string_view newName);
    void Clear();

    // Capture one input frame. Call once per frame before querying edge states.
    void Update();
    void Update(const std::vector<SDL_Scancode>& downKeys);

    [[nodiscard]] bool IsActionDown(std::string_view action) const;
    [[nodiscard]] bool WasActionPressed(std::string_view action) const;
    [[nodiscard]] bool WasActionReleased(std::string_view action) const;
    [[nodiscard]] const std::vector<SDL_Scancode>& GetBindings(std::string_view action) const;
    [[nodiscard]] const std::unordered_map<std::string, std::vector<SDL_Scancode>>&
        GetAllBindings() const noexcept { return m_bindings; }

    [[nodiscard]] static InputMap CreateDefaultMovementMap();

private:
    struct ActionState
    {
        bool previousDown = false;
        bool currentDown = false;
    };

    std::unordered_map<std::string, std::vector<SDL_Scancode>> m_bindings;
    std::unordered_map<std::string, ActionState> m_actionStates;
};
} // namespace ember
