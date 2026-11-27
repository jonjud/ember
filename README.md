# EMBER Engine

A lightweight, 2D-first game engine written in modern C++.

EMBER is in early development. It currently includes an SDL3-backed 2D engine foundation, scene/entity model, project asset pipeline, JSON scene/project persistence, and an ImGui editor with a basic Play Mode.

## Project status

**Early development — 2D core and editor foundation.** See [ROADMAP.md](docs/ROADMAP.md) for planned work and [ARCHITECTURE.md](docs/ARCHITECTURE.md) for current design and known gaps.

## Technology

- C++20
- CMake 3.24+
- SDL3 3.2.10 and SDL3_image
- Dear ImGui (editor only)
- nlohmann/json

## Current capabilities

- SDL application lifecycle, window, event loop, timing, and keyboard input.
- Renderer2D with primitives, sprites, camera transforms, and BMP/PNG/JPEG loading.
- Scene/Entity model, Transform2D, RenderComponent2D, and per-entity MovementComponent2D.
- Reference-counted texture cache and project-facing AssetManager.
- Stable-ID asset metadata, registry scanning, diagnostics, and texture import settings.
- Versioned .emberproject configuration and .emberscene scene serialization.
- Editor panels: Scene Hierarchy, Inspector, Asset Browser, Project Settings, and interactive 2D viewport.
- Move/Rotate/Scale gizmos, snapping, Local/World orientation, entity duplicate/delete.
- Isolated Play/Stop workflow and basic keyboard movement.
- Persistent editor input action settings.

## Build and test

Requirements:
- CMake 3.24 or newer
- A C++20 compiler
- Git (for fetching dependencies through CMake FetchContent)

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Run the editor:
- Windows: `build/Debug/ember.exe` (multi-config generators) or `build/ember.exe`
- Linux/macOS: `./build/ember`

## Repository layout

```text
EMBER/
├── .github/workflows/
├── docs/
├── src/
│   ├── Editor/
│   ├── Engine/
│   │   ├── Core/
│   │   ├── Math/
│   │   ├── Platform/
│   │   ├── Project/
│   │   ├── Renderer/
│   │   ├── Resources/
│   │   └── Scene/
│   └── main.cpp
├── tests/
└── CMakeLists.txt
```

## Design principles

1. Keep the core modular and editor code separate from runtime code.
2. Prefer understandable code over premature abstraction.
3. Build runnable vertical slices and validate them with tests.
4. Isolate third-party dependencies behind narrow interfaces where useful.
5. Treat 2D as the first-class target; consider 3D only after the foundation is stable.

## License

License selection is pending. Third-party dependencies retain their own licenses.
