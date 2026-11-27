#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
namespace ember
{
class InputMap;
class InputActionState
{
public:
    [[nodiscard]] bool IsDown(std::string_view action) const
    {
        const auto it = m_states.find(std::string(action));
        return it != m_states.end() && it->second.currentDown;
    }
    [[nodiscard]] bool WasPressed(std::string_view action) const
    {
        const auto it = m_states.find(std::string(action));
        return it != m_states.end() && it->second.currentDown && !it->second.previousDown;
    }
    [[nodiscard]] bool WasReleased(std::string_view action) const
    {
        const auto it = m_states.find(std::string(action));
        return it != m_states.end() && !it->second.currentDown && it->second.previousDown;
    }
private:
    friend class InputMap;
    struct State { bool previousDown = false; bool currentDown = false; };
    std::unordered_map<std::string, State> m_states;
};
} // namespace ember
