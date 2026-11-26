#include "Engine/Platform/InputMap.hpp"

#include <cstdlib>
#include <iostream>

namespace
{
int g_failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        ++g_failures;
    }
}
}

int main()
{
    ember::InputMap map;
    Check(!map.BindAction("", SDL_SCANCODE_A), "Empty action names are rejected");
    Check(!map.BindAction("Move", SDL_SCANCODE_UNKNOWN), "Unknown keys are rejected");
    Check(map.BindAction("MoveLeft", SDL_SCANCODE_A), "A key can be bound to an action");
    Check(!map.BindAction("MoveLeft", SDL_SCANCODE_A), "Duplicate bindings are rejected");
    Check(map.BindAction("MoveLeft", SDL_SCANCODE_LEFT), "An action can have multiple keys");
    Check(map.GetBindings("MoveLeft").size() == 2, "Bindings can be queried");
    Check(map.GetBindings("Missing").empty(), "Missing actions return no bindings");
    Check(map.UnbindAction("MoveLeft", SDL_SCANCODE_A), "A key can be unbound");
    Check(map.GetBindings("MoveLeft").size() == 1, "Unbinding preserves other keys");
    Check(!map.UnbindAction("MoveLeft", SDL_SCANCODE_A), "Missing bindings cannot be unbound");

    map.ClearAction("MoveLeft");
    Check(map.GetBindings("MoveLeft").empty(), "An action can be cleared");

    const ember::InputMap defaults = ember::InputMap::CreateDefaultMovementMap();
    Check(defaults.GetBindings("MoveLeft").size() == 2, "Default map supports A and Left");
    Check(defaults.GetBindings("MoveRight").size() == 2, "Default map supports D and Right");
    Check(defaults.GetBindings("MoveUp").size() == 2, "Default map supports W and Up");
    Check(defaults.GetBindings("MoveDown").size() == 2, "Default map supports S and Down");

    (void)map.BindAction("Jump", SDL_SCANCODE_SPACE);
    map.Clear();
    Check(map.GetBindings("Jump").empty(), "All actions can be cleared");

    if (g_failures != 0)
    {
        std::cerr << g_failures << " input map test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All input map tests passed.\n";
    return EXIT_SUCCESS;
}
