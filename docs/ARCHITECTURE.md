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
│       ├── Entity.hpp
│       ├── RenderComponent2D.hpp
│       ├── Scene.hpp/.cpp
│       └── Transform2D.hpp
└── main.cpp
```

## Current responsibilities

- **Core / Application:** owns the SDL window, active scene, and event/update/render loop.
- **Core / Time:** measures frame delta time using SDL's high-resolution performance counter.
- **Platform / Input:** exposes keyboard state through an engine-facing API.
- **Math / Vector2:** engine-owned 2D vector type, independent of SDL.
- **Scene / Entity:** provides a stable 64-bit entity ID, name, active state, Transform2D, and RenderComponent2D.
- **Scene / Scene:** owns entities, creates and destroys them, looks them up by ID, and iterates without allocating a temporary list. Entity references remain stable when other entities are inserted; destroying that entity invalidates its reference.
- **Scene / Transform2D:** stores an object's world position, scale, and rotation in radians.
- **Scene / RenderComponent2D:** stores filled-rectangle size, RGBA color, and visibility.
- **Renderer / Camera2D:** converts world positions and sizes into screen coordinates and applies camera zoom and rotation.
- **Renderer / Renderer2D:** owns the SDL renderer, draws primitives and sprites, and manages loaded texture lifetimes.
- **main.cpp:** contains only the process entry point.

## Scene and entity workflow

- Construct a `Scene`, optionally providing its name.
- Call `CreateEntity(name)` to create an entity and receive a stable reference.
- Read or update its transform through `GetTransform()`.
- Configure its simple visual through `GetRenderComponent()`.
- Use `GetId()` to retain its identity and `FindEntity(id)` to look it up later.
- Call `DestroyEntity(id)` to remove one entity or `Clear()` to remove all entities.
- Use `ForEachEntity(callback)` to update or render entities without allocating a temporary list.
- Entity IDs are monotonically assigned and are not reused during a scene's lifetime. ID exhaustion is reported with `std::overflow_error`.

## Runtime scene rendering

The application owns a scene. Its sample player is an ordinary entity with a Transform2D and RenderComponent2D. Input updates the entity transform, and the render loop draws every active entity whose render component is visible. The scene does not depend on SDL; SDL-specific drawing remains inside Renderer2D.

## Renderer texture workflow

- `LoadTexture(path)` loads a BMP image through SDL3 and returns an engine-owned `TextureHandle`.
- `DrawTexture(handle, x, y, width, height)` draws the texture into a destination rectangle.
- `DestroyTexture(handle)` releases an individual texture.
- `Shutdown()` releases every remaining texture before destroying the SDL renderer.

`DrawFilledRect(transform, camera, size, color)` renders a transformed rectangle using world coordinates, object scale, object rotation, and the camera conversion. `DrawTexture(handle, transform, camera, size)` applies the same position, scale, and rotation workflow to sprites. Transform and camera rotations are stored in radians; the renderer converts the relative sprite rotation to degrees for SDL3.

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

Add component-oriented tests and expand RenderComponent2D to support texture-backed sprites while keeping SDL details inside the renderer.
