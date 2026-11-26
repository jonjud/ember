# EMBER Engine Architecture

EMBER is a 2D-first engine written in C++20. The architecture is intentionally small and grows only when a working use case requires another subsystem.

## Current executable

The `ember` executable is the application entry point. It creates an `ember::Application` and delegates initialization, the main loop, and shutdown to the engine library.

## Current source layout

```text
src/
├── Engine/
│   ├── Core/
│   │   ├── Application.hpp/.cpp
│   │   └── Time.hpp/.cpp
│   ├── Math/
│   │   └── Vector2.hpp
│   ├── Platform/
│   │   └── Input.hpp/.cpp
│   ├── Renderer/
│   │   ├── Camera2D.hpp/.cpp
│   │   └── Renderer2D.hpp/.cpp
│   └── Scene/
│       └── Transform2D.hpp
└── main.cpp
```

## Current responsibilities

- **Core / Application:** owns the SDL window and runs the event/update/render loop.
- **Core / Time:** measures frame delta time using SDL's high-resolution performance counter.
- **Platform / Input:** exposes keyboard state through an engine-facing API.
- **Math / Vector2:** engine-owned 2D vector type, independent of SDL.
- **Scene / Transform2D:** stores an object's world position and scale.
- **Renderer / Camera2D:** converts world positions and sizes into screen coordinates and applies camera zoom.
- **Renderer / Renderer2D:** owns the SDL renderer, draws primitives and sprites, and manages loaded texture lifetimes.
- **main.cpp:** contains only the process entry point.

## Renderer texture workflow

- `LoadTexture(path)` loads a BMP image through SDL3 and returns an engine-owned `TextureHandle`.
- `DrawTexture(handle, x, y, width, height)` draws the texture into a destination rectangle.
- `DestroyTexture(handle)` releases an individual texture.
- `Shutdown()` releases every remaining texture before destroying the SDL renderer.

`DrawFilledRect(transform, camera, size, color)` renders a rectangle using world coordinates, object scale, and the camera conversion. The current camera supports position and zoom; rotation is intentionally deferred.

Texture handles are lightweight IDs; SDL texture pointers remain private to the renderer. The first loader intentionally supports BMP only, avoiding another third-party dependency while the resource API is established. PNG/JPEG support can be added later behind the same interface.

## Planned layers

```text
Editor
  └── Engine Runtime
        ├── Core       (application, time, logging)
        ├── Platform   (window, input)
        ├── Renderer   (2D drawing, textures, camera)
        ├── Scene      (entities, components, serialization)
        ├── Physics    (2D collision and simulation)
        ├── Audio
        └── Resources  (asset loading and lifetime)
```

## Dependency policy

- SDL3: windowing, event handling, keyboard input, initial rendering backend, and BMP decoding.
- Dear ImGui: planned editor interface; not part of the runtime core.
- Box2D: planned 2D physics backend.
- miniaudio: planned audio backend.

Third-party APIs should be hidden behind narrow engine-facing interfaces when doing so provides a concrete portability, testing, or maintenance benefit.

## Next milestone

Extend the current transform and camera APIs with rotation, then apply camera-aware transforms to sprite rendering before introducing scenes and entities.
