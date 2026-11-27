# UI Runtime — Windows Editor Verification

Run this checklist once the UI System implementation is complete. Automated CI covers component validation/serialization, Canvas layout, pointer hit testing, pointer capture, click generation, and runtime Component callback dispatch; this checklist covers the interactive Editor workflow.

## Setup and authoring
- [ ] Launch the Windows Editor and open or create a test Scene.
- [ ] Create a Canvas Entity and add `UICanvas2D`.
- [ ] Confirm the default reference resolution is 1280 × 720; change it and confirm the viewport preview scales while preserving aspect ratio.
- [ ] Add a Button Entity with `UIButton2D`; set its label, position, size and color in the Inspector.
- [ ] Save the Scene, close/reopen it, and confirm Canvas/Button properties persist.

## Play Mode interaction
- [ ] Enter Play Mode (F5); confirm the UI overlay is visible inside the Canvas frame.
- [ ] Move the pointer over the Button; confirm the hover visual appears.
- [ ] Press and hold the left mouse button; confirm the pressed visual appears.
- [ ] Drag outside while holding; confirm the Button remains captured/pressed but is no longer hovered.
- [ ] Release outside; confirm no click is reported.
- [ ] Press and release inside the Button; confirm the status bar reports `UI Button clicked` for the expected Entity.
- [ ] Disable or make the Button non-interactable; confirm it no longer responds.
- [ ] Resize the viewport; confirm letterboxing and pointer hit testing remain aligned.
- [ ] Stop Play Mode (Shift+F5); confirm transient hover/pressed state clears and the edit Scene remains unchanged.

## Result
- Date:
- Windows version:
- Result: Pass / Fail
- Notes / screenshots:
