#include "Editor/SceneUndoHistory.hpp"

#include <iostream>
#include <string>

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
    else
    {
        std::cout << "[PASS] " << description << '\n';
    }
}
}

int main()
{
    ember::SceneUndoHistory history(3, 32);
    history.Reset("A");
    Check(!history.CanUndo() && !history.CanRedo(), "Reset establishes a single baseline snapshot");

    Check(history.Record("B"), "A changed snapshot is recorded");
    Check(history.Record("C"), "A second snapshot is recorded");
    Check(history.CanUndo(), "Undo is available after recording changes");
    Check(*history.Undo() == "B", "Undo returns the previous snapshot");
    Check(history.CanRedo(), "Redo is available after undo");
    Check(*history.Redo() == "C", "Redo returns the next snapshot");

    Check(!history.Record("C"), "Identical snapshots are ignored");
    Check(history.Record("D"), "New edits can be recorded");
    Check(history.GetEntryCount() == 3, "History evicts the oldest entry at its count limit");
    Check(*history.Undo() == "C", "Undo remains valid after count-based eviction");

    Check(history.Record("E"), "Recording after undo creates a new branch");
    Check(!history.CanRedo(), "A new edit discards the redo branch");

    const auto bytesBeforeOversize = history.GetBytesUsed();
    Check(!history.Record(std::string(33, 'x')), "Snapshots larger than the memory budget are rejected");
    Check(history.GetBytesUsed() == bytesBeforeOversize, "Rejected snapshots do not change history");

    ember::SceneUndoHistory byteLimited(10, 4);
    byteLimited.Reset("a");
    Check(byteLimited.Record("bb"), "Small snapshot fits the byte budget");
    Check(byteLimited.Record("cc"), "Second small snapshot fits the byte budget");
    Check(byteLimited.GetBytesUsed() <= 4, "History stays within its byte budget");

    ember::SceneUndoHistory disabled(0, 0);
    disabled.Reset("x");
    Check(disabled.GetEntryCount() == 0, "Zero-capacity history safely disables recording");

    if (failures != 0)
    {
        std::cerr << failures << " undo history test(s) failed.\n";
        return 1;
    }
    std::cout << "All scene undo history tests passed.\n";
    return 0;
}
