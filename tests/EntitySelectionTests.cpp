#include "Editor/EntitySelection.hpp"

#include <iostream>
#include <vector>

namespace
{
int failures = 0;
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << description << '\n';
        ++failures;
    }
    else std::cout << "[PASS] " << description << '\n';
}
}

int main()
{
    ember::EntitySelection selection;
    const std::vector<ember::EntityId> order{10, 20, 30, 40, 50};

    selection.SelectOnly(20);
    Check(selection.Size() == 1 && selection.Primary() == 20, "Single selection sets primary and anchor");
    selection.Toggle(40);
    Check(selection.Size() == 2 && selection.Contains(20) && selection.Contains(40),
        "Ctrl-style toggle adds an Entity");
    Check(selection.Primary() == 40, "Newly toggled Entity becomes primary");
    selection.Toggle(40);
    Check(selection.Size() == 1 && selection.Primary() == 20, "Toggling selected Entity removes it");

    selection.SelectOnly(20);
    selection.SelectRange(order, 40, false);
    Check(selection.Size() == 3 && selection.Contains(20) && selection.Contains(30) &&
        selection.Contains(40), "Shift-style range selects inclusive ordered range");
    Check(selection.Primary() == 40, "Range target becomes primary");

    selection.SelectRange(order, 50, true);
    Check(selection.Size() == 4 && selection.Contains(50), "Additive range preserves prior selection");
    selection.SelectAll(order);
    Check(selection.Size() == order.size() && selection.Primary() == 50,
        "Select All captures every Entity in order");

    selection.Prune([](ember::EntityId id) { return id != 30 && id != 50; });
    Check(selection.Size() == 3 && !selection.Contains(30) && !selection.Contains(50),
        "Prune removes deleted Entity IDs");
    Check(selection.Primary() == 40, "Prune selects a surviving primary");
    selection.Clear();
    Check(selection.Empty() && selection.Primary() == 0, "Clear resets selection state");

    if (failures != 0) return 1;
    std::cout << "All entity selection tests passed.\n";
    return 0;
}
