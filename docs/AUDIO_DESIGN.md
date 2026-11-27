# Audio Runtime Design

## Goal
Provide a small runtime-facing 2D audio foundation while keeping miniaudio types and headers private to the implementation.

## Dependency and boundary
- Pin miniaudio 0.11.26 through CMake FetchContent.
- Compile the miniaudio implementation once in `ember_miniaudio`.
- `AudioEngine` is the only public runtime boundary; its header exposes no miniaudio types.
- The Editor remains independent of audio backend objects.

## Initial vertical slice
- Initialize and shut down an audio engine with deterministic ownership.
- Support device-backed runtime initialization and device-less initialization for automated tests/offline processing.
- Load a sound from a filesystem path, optionally streaming it.
- Return an engine-owned `AudioSoundHandle` for Play, Stop, IsPlaying, SetVolume, SetLooping and UnloadSound.
- Reject invalid handles, invalid initialization parameters, non-finite volume, and volume outside [0, 4].
- Unload every sound before shutting down the mixer.

## Lifetime rules
- Handles are valid only for the AudioEngine instance that created them and until UnloadSound or Shutdown.
- AudioEngine is non-copyable and movable.
- Destroying or shutting down an engine releases all loaded sounds before releasing miniaudio's engine.
- Scene serialization never stores runtime audio handles. Serializable audio authoring components and Editor controls are later slices.

## Test strategy
- Use miniaudio's device-less engine mode so CI does not require a physical playback device.
- Generate a tiny WAV fixture in the test process; do not depend on external sample assets.
- Cover initialization validation, load failure, playback control, volume bounds, looping, unload, and shutdown/reinitialize behavior.
