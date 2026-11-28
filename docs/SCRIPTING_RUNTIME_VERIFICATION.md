# Combined Windows Editor Verification — Input Actions and Lua Scripting

> Run this checklist after pulling the latest `develop` branch. Automated CI covers the underlying engine/runtime behavior; this document verifies the complete interactive Editor workflow. Keep a copy of the project before testing.

## 1. Build and startup
- [ ] Pull latest `develop`, configure/build the Editor, and launch it.
- [ ] Open a project with a writable Assets directory.
- [ ] Confirm the Editor opens without startup errors.

## 2. Input Action Mapping
- [ ] Open **Input Settings** and confirm defaults: MoveLeft (A/Left), MoveRight (D/Right), MoveUp (W/Up), MoveDown (S/Down).
- [ ] Rebind one action and add a secondary key; confirm both are displayed.
- [ ] Attempt to bind a key already used by another action; confirm conflict is rejected without changing either action.
- [ ] Start key capture and press Esc; confirm capture cancels.
- [ ] Clear an action and confirm it becomes Unbound.
- [ ] Create a custom action, rename it, then delete it; confirm the list updates correctly.
- [ ] Reset to defaults and confirm custom actions are removed.
- [ ] Enter Play Mode with a Movement-enabled Entity; verify WASD and arrow-key movement.
- [ ] Change one binding, close/reopen the Editor, and confirm it persists. Restore defaults afterward.

## 3. Lua Script Asset and source editor
- [ ] Place a small `.lua` source file in the project's Assets directory, then refresh the Asset Browser.
- [ ] Select the imported Script asset and confirm its language is identified as Lua 5.5.1 / `lua`.
- [ ] Open **Window → Script Source Editor**, or use **Open in Script Source Editor** from the selected Script asset.
- [ ] Replace the source with the smoke-test script below, then press **Validate** and confirm syntax validation succeeds.
- [ ] Save the source and confirm the dirty indicator/status clears.

Smoke-test script:
```lua
function OnCreate()
    entity:set_position(100, 100)
end

function OnUpdate(deltaSeconds)
    local x, y = entity:get_position()
    entity:set_position(x + deltaSeconds * 60, y)
end
```

## 4. Lua assignment and Play Mode
- [ ] Create/select a test Entity and add a **ScriptComponent**.
- [ ] Assign the imported Lua Script asset in the Script property.
- [ ] Confirm the Inspector indicates the Lua backend is available.
- [ ] Save the Scene, then press F5 to enter Play Mode.
- [ ] Confirm the test Entity moves horizontally while Play Mode runs.
- [ ] Press Shift+F5 to stop; confirm the authoring Scene returns to its saved position and no runtime state leaked into it.
- [ ] Re-enter Play Mode once to confirm the script instance initializes again.

## 5. Diagnostics and editor safety
- [ ] Temporarily introduce a Lua syntax error, validate, and confirm line/column diagnostics appear; restore valid source afterward.
- [ ] Temporarily introduce a runtime error in a callback, enter Play Mode, and open **Window → Script Diagnostics**; confirm the failure is reported and the Editor remains responsive. Restore the valid script afterward.
- [ ] Make an unsaved source edit and attempt to close Script Source Editor; confirm it refuses to close until changes are saved or discarded.
- [ ] Make an unsaved source edit and request application exit; confirm **Save and Exit / Discard and Exit / Cancel** are offered.
- [ ] With a clean source buffer, modify the source file externally; confirm saving the stale buffer is blocked and reload is available.
- [ ] Confirm no unexpected files remain (for example, temporary `.ember-tmp` files) and the project opens normally after restart.

## 6. Result
- [ ] Input Action Mapping checks passed.
- [ ] Lua Script authoring and Play Mode checks passed.
- [ ] Diagnostics and unsaved-work protection checks passed.
- [ ] Any failure recorded with the exact step, expected result, observed result, and screenshot/log where possible.

**Pass criteria:** all checks above pass on Windows. CI success alone does not close this interactive verification; report the result separately.
