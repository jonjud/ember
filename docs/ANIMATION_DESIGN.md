# 2D Sprite Animation Design

## Status
- Active implementation slice: SpriteAnimation2D component, frame playback, serialization, Inspector settings, Editor viewport preview and Renderer2D source-region support.
- Windows CI #193 is pending; manual Editor verification is deferred until the planned final test session.

## Authoring model
- One SpriteAnimation2D component describes a contiguous row-major frame sequence in the texture assigned through RenderComponent2D.
- Frame origin is the top-left of the sprite sheet.
- Authored fields: frame width/height in pixels, number of columns, total frame count, frames per second, loop and playing.
- The component does not own or serialize texture handles; RenderComponent2D continues to reference the texture by stable asset ID.

## Playback behavior
- Scene::Update drives the component through the existing Component::OnUpdate lifecycle.
- OnCreate resets the current frame and elapsed time.
- Playback uses a time accumulator and supports multiple frame advances in one update.
- Looping animations wrap to frame zero. Non-looping animations stop on their last frame.
- Playback delta is capped at 0.25 seconds per component update; frames per second are limited to 1000 and authored frame configuration is validated.
- Clone preserves authoring configuration but resets transient current-frame and elapsed-time state.

## Rendering integration
- SpriteAnimation2D exposes its current source position and size in texture pixels.
- Renderer2D has a source-region DrawTexture overload for runtime callers.
- Editor viewport derives normalized UV coordinates from the preview texture dimensions and displays the current frame while Play Mode updates the Scene.
- If the selected frame rectangle does not fit inside the source texture, the Editor currently falls back to showing the full texture.

## Current limitations
- One contiguous clip per component; no named clip library or AnimationClip asset yet.
- No frame events, keyframes, blend trees, transitions, timeline editor, or per-frame durations.
- Editor playback is driven by Play Mode; no dedicated animation preview transport yet.

## Next slices
1. Confirm CI and harden playback edge cases.
2. Add named AnimationClip assets and multiple clips per entity.
3. Add animation preview controls and frame-grid validation in the Inspector.
4. Add frame events and transitions only after the clip model is stable.
