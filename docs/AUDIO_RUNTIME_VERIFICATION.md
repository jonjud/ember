# Audio Runtime — Windows Editor Verification

> Run this checklist once after pulling the completed Audio implementation. Automated Windows CI #219 is green; this checklist covers interactive Editor behavior that CI cannot fully verify.

## 1. Launch and device handling
- [ ] Launch the EMBER Editor successfully.
- [ ] Confirm the Editor reports audio-device initialization status.
- [ ] With an available output device, confirm audio is initialized without blocking Editor startup.
- [ ] If testing without an output device, confirm the Editor remains usable and reports a warning rather than crashing.

## 2. Audio asset authoring
- [ ] Import a supported audio file (WAV is a good first test) through the Asset Browser.
- [ ] Create/select an Entity and add an `AudioSource2D` component.
- [ ] Open the Inspector Audio asset picker and assign the imported Audio asset.
- [ ] Change Volume, Loop, Play On Start, Streaming, and Enabled; confirm the controls retain their values.
- [ ] Save the Scene, close/reopen it, and confirm the AudioSource2D settings and asset assignment persist.

## 3. Play Mode runtime playback
- [ ] Enter Play Mode with Play On Start enabled and confirm the assigned sound plays.
- [ ] Select the AudioSource2D Entity in Scene Hierarchy.
- [ ] Confirm the Audio Runtime panel shows the assigned asset and offers Play Audio / Stop Audio controls.
- [ ] Use Play Audio and Stop Audio; confirm playback starts and stops.
- [ ] Confirm Scene audio diagnostics are visible when a referenced asset cannot be resolved.
- [ ] Stop Play Mode and confirm runtime audio stops and its handles are released.

## 4. Asset Browser preview
- [ ] Select an Audio asset in Asset Browser and use Play Preview; confirm it plays once.
- [ ] Use Stop Preview and confirm playback stops.
- [ ] Start a different preview or change selection; confirm the previous preview is cleaned up.
- [ ] Refresh assets, enter Play Mode, and close the Editor; confirm no preview continues unexpectedly.

## 5. Graceful failure behavior
- [ ] Assign a missing/invalid Audio reference and confirm a diagnostic appears without crashing.
- [ ] If no audio device is available, confirm preview and runtime playback remain unavailable gracefully while other Editor functions continue.
- [ ] Confirm the Editor can still save and reopen the Scene when an AudioSource2D reference is invalid.

## Completion
- [ ] All applicable checks above pass.
- [ ] Record any device-specific limitation or failure before closing the Audio milestone.
