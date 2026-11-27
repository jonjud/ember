# Input Action Mapping — Windows Editor Verification

> Implementation and automated CI are complete. Run this checklist once on Windows before closing the milestone. No code changes are required for this verification.

## 1. Startup and default bindings
- [ ] Pull the latest `develop` branch and build the Editor.
- [ ] Launch the Editor and open the **Input Settings** panel.
- [ ] Confirm the default movement actions are present:
  - `MoveLeft`: A and Left Arrow
  - `MoveRight`: D and Right Arrow
  - `MoveUp`: W and Up Arrow
  - `MoveDown`: S and Down Arrow

## 2. Rebinding and conflict handling
- [ ] Rebind `MoveLeft` to a different key; confirm the displayed binding changes.
- [ ] Add a second key with **+Key**; confirm both bindings are displayed.
- [ ] Try assigning a key already used by another action; confirm the conflict is rejected and the original binding remains unchanged.
- [ ] Start rebinding and press Esc; confirm capture is cancelled without changing the binding.
- [ ] Clear an action; confirm it displays as **Unbound**.

## 3. Custom actions
- [ ] Create a custom action with **Add + Bind** and assign a key.
- [ ] Confirm the custom action appears in the action table.
- [ ] Rename the custom action; confirm its bindings remain attached to the renamed action.
- [ ] Delete the custom action; confirm it disappears from the table.
- [ ] Use **Reset All to Defaults**; confirm custom actions are removed and default movement bindings return.

## 4. Runtime movement and persistence
- [ ] Select an Entity with Movement enabled and a positive speed.
- [ ] Enter Play Mode (F5); verify movement using WASD.
- [ ] Verify the arrow keys also move the Entity.
- [ ] Stop Play Mode (Shift+F5); rebind one movement key and confirm the setting is saved.
- [ ] Close and relaunch the Editor; confirm the customized binding persists.
- [ ] Restore defaults and confirm the default bindings persist after relaunch.

## 5. Result
- [ ] All checks passed.
- [ ] Any failure has been noted with the exact step and observed behavior.

Automated CI already covers action-state Down/Pressed/Released transitions, project configuration validation/persistence, and forwarding action state to runtime Components. This checklist verifies the interactive Editor workflow and Play Mode behavior.
