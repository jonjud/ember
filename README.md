# EMBER Engine

A lightweight, 2D-first game engine written in modern C++.

EMBER is being built from the ground up. The project currently has a modular desktop foundation: application lifecycle, frame timing, keyboard input, and an SDL-backed 2D renderer with BMP texture loading and sprite drawing.

## Project status

**Early development — Phase 2: 2D Core**

See [ROADMAP.md](docs/ROADMAP.md) for planned milestones and [ARCHITECTURE.md](docs/ARCHITECTURE.md) for the current design.

## Technology

- C++20
- CMake 3.24+
- SDL3

## Current prototype

- Opens a resizable 1280×720 window.
- Moves a test rectangle with WASD or arrow keys.
- Uses frame delta time for consistent movement speed.
- Normalizes diagonal movement.
- Uses a dedicated Renderer2D module for drawing.
- Loads BMP images into engine-managed texture handles.
- Provides a sprite drawing API without exposing SDL texture pointers.
- Requests VSync where supported.
- Press Escape to quit.

## Texture API

The renderer currently supports BMP files through SDL3:

```cpp
const ember::TextureHandle sprite = renderer.LoadTexture("assets/player.bmp");
if (sprite)
{
    renderer.DrawTexture(sprite, 100.0F, 100.0F, 64.0F, 64.0F);
}

// Release early when the texture is no longer needed.
// Remaining textures are released automatically by Renderer2D::Shutdown().
renderer.DestroyTexture(sprite);
```

PNG/JPEG support and a centralized asset cache are planned follow-up work.

## Build

Requirements:
- CMake 3.24 or newer
- A C++20 compiler
- Git (for fetching SDL3 through CMake FetchContent)

```sh
cmake -S . -B build
cmake --build build --config Debug
```

Run the executable:

- Windows: `build/Debug/ember.exe` (multi-config generators) or `build/ember.exe`
- Linux/macOS: `./build/ember`

## Repository layout

```text
EMBER/
├── .github/
│   └── workflows/
├── docs/
├── src/
│   ├── Engine/
│   │   ├── Core/
│   │   ├── Platform/
│   │   └── Renderer/
│   └── main.cpp
├── .gitignore
└── CMakeLists.txt
```

## Design principles

1. Keep the core small and modular.
2. Prefer understandable code over premature abstraction.
3. Build runnable vertical slices and test each milestone.
4. Keep third-party dependencies isolated.
5. Treat 2D as the first-class target; consider 3D only after the foundation is stable.

## License

License selection is pending. Third-party dependencies retain their own licenses.
