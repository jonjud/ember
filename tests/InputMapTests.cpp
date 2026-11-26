#include "Engine/Platform/InputMap.hpp"
#include "Engine/Platform/Input.hpp"

#include <cstdlib>
#include <iostream>

// Map data tests do not require SDL keyboard initialization.
namespace ember
{
bool Input::IsKeyDown(SDL_Scancode)
{
    return false;
}
}

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
    Check(!map.CreateAction(""), "Empty action names are rejected");
    Check(map.CreateAction("EmptyAction"), "An action can exist before it has a key");
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
    Check(map.BindAction("Jump", SDL_SCANCODE_SPACE), "Custom action can be created");
    Check(map.CreateAction("Occupied"), "An existing destination action can be created");
    Check(!map.RenameAction("Jump", "Occupied"), "Actions cannot be renamed to an existing name");
    Check(map.RenameAction("Jump", "Interact"), "Custom action can be renamed");
    Check(map.RenameAction("EmptyAction", "Pause"), "Unbound custom actions can be renamed");
    Check(map.GetBindings("Interact").size() == 1 &&
          map.GetBindings("Interact").front() == SDL_SCANCODE_SPACE,
          "Renaming preserves action bindings");
    Check(!map.RenameAction("Missing", "Other"), "Missing actions cannot be renamed");

    const ember::InputMap defaults = ember::InputMap::CreateDefaultMovementMap();
    Check(defaults.GetBindings("MoveLeft").size() == 2, "Default map supports A and Left");
    Check(defaults.GetBindings("MoveRight").size() == 2, "Default map supports D and Right");
    Check(defaults.GetBindings("MoveUp").size() == 2, "Default map supports W and Up");
    Check(defaults.GetBindings("MoveDown").size() == 2, "Default map supports S and Down");

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
