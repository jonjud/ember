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
│   ├── Platform/
│   │   └── Input.hpp/.cpp
│   └── Renderer/
│       └── Renderer2D.hpp/.cpp
└── main.cpp
```

## Current responsibilities

- **Core / Application:** owns the SDL window and runs the event/update/render loop.
- **Core / Time:** measures frame delta time using SDL's high-resolution performance counter.
- **Platform / Input:** exposes keyboard state through an engine-facing API.
- **Renderer / Renderer2D:** owns the SDL renderer and provides clear, filled-rectangle, and present operations.
- **main.cpp:** contains only the process entry point.

The current render test is a movable rectangle. WASD and arrow keys move it; Escape exits. Movement uses delta time and is normalized diagonally. The renderer requests VSync to avoid an unrestricted render loop where the platform supports it.

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

- SDL3: windowing, event handling, keyboard input, and initial rendering backend.
- Dear ImGui: planned editor interface; not part of the runtime core.
- Box2D: planned 2D physics backend.
- miniaudio: planned audio backend.

Third-party APIs should be hidden behind narrow engine-facing interfaces when doing so provides a concrete portability, testing, or maintenance benefit.

## Next milestone

Add a texture resource abstraction and sprite drawing. Keep image loading and lifetime management out of the application loop.
