# Audio Runtime Design

## Goal
Provide a small runtime-facing 2D audio foundation while keeping miniaudio types and headers private to the implementation.

## Dependency and boundary
- Pin miniaudio 0.11.26 through CMake FetchContent.
- Compile miniaudio's upstream `miniaudio.c` implementation once in `ember_miniaudio`.
- `AudioEngine` is the only public runtime boundary; its header exposes no miniaudio types.
- The Editor remains independent of audio backend objects.

## Initial vertical slice
- Initialize and shut down an audio engine with deterministic ownership. Application and Editor initialize it on startup and release it before SDL shutdown; audio-device failure is non-fatal.
- Support device-backed runtime initialization and device-less initialization for automated tests/offline processing.
- Load a sound from a filesystem path, optionally streaming it.
- Return an engine-owned `AudioSoundHandle` for Play, Stop, IsPlaying, SetVolume, SetLooping and UnloadSound.
- Reject invalid handles, invalid initialization parameters, non-finite volume, and volume outside [0, 4].
- Unload every sound before shutting down the mixer.

## Lifetime rules
- Handles are valid only for the AudioEngine instance that created them and until UnloadSound or Shutdown. IDs remain monotonic across shutdown/reinitialize cycles so stale handles cannot alias newly loaded sounds.
- AudioEngine is non-copyable and non-movable so backend ownership cannot be accidentally abandoned.
- Destroying or shutting down an engine releases all loaded sounds before releasing miniaudio's engine.
- Scene serialization never stores runtime audio handles. Serializable audio authoring components, Scene-owned runtime playback controls, and Editor preview controls are layered on top of this boundary.

## Test strategy
- Use miniaudio's device-less engine mode so CI does not require a physical playback device.
- Generate a tiny WAV fixture in the test process; do not depend on external sample assets.
- Cover initialization validation, load failure, playback control, volume bounds, looping, unload, and shutdown/reinitialize behavior.


## Editor authoring and diagnostics
- The AudioSource2D Inspector presents a project Audio-asset picker backed by AssetRegistry instead of requiring users to type stable asset IDs. Only Audio assets with safely resolved, existing source files are offered.
- Scene startup reports a diagnostic when an enabled, active AudioSource2D has no AudioEngine or AssetRegistry. Disabled and inactive sources are intentionally skipped without loading sounds.
