# Initial Architecture

The first version deliberately keeps the architecture small.

## Current executable

`ember_editor` is currently a bootstrap executable. It initializes SDL3, creates a resizable window, processes quit events, and draws a temporary rectangle.

The executable is not yet a full editor.

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

- SDL3: initial platform, input, and rendering foundation.
- Dear ImGui: planned editor interface; not part of the runtime core.
- Box2D: planned 2D physics backend.
- miniaudio: planned audio backend.

Third-party code should remain behind narrow engine-facing interfaces where that provides a real portability or testing benefit.

## Immediate next step

Build and run the bootstrap application on the target development machine. Once the window and render test are confirmed, extract the application loop and input handling into small engine modules.
