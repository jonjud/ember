# EMBER Engine

A lightweight, 2D-first game engine written in modern C++.

EMBER is being built from the ground up. The project currently has a modular desktop foundation: application lifecycle, frame timing, keyboard input, an SDL-backed 2D renderer, scene/entity support, and a reference-counted texture cache.

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
- Renders texture-backed scene entities as sprites with transform and tint support.
- Reuses loaded textures through a path-based reference-counted cache.
- Requests VSync where supported.
- Press Escape to quit.

## Texture API

For direct renderer ownership:

```cpp
const ember::TextureHandle sprite = renderer.LoadTexture("assets/player.bmp");
if (sprite)
{
    renderer.DrawTexture(sprite, 100.0F, 100.0F, 64.0F, 64.0F);
    renderer.DestroyTexture(sprite);
}
```

For shared assets, configure a `TextureCache` with renderer-backed load and destroy callbacks. Each successful `Load(path)` must be paired with one `Release(path)`. Repeated loads of the same normalized path return the same handle; the texture is destroyed after its final reference is released. Clear the cache before shutting down its renderer.

The cache deliberately does not own scene components' references. A component may borrow a handle, so release the corresponding asset only after no scene object needs to draw it.

## Build

Requirements:
- CMake 3.24 or newer
- A C++20 compiler
- Git (for fetching SDL3 through CMake FetchContent)

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
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
│   │   ├── Math/
│   │   ├── Platform/
│   │   ├── Renderer/
│   │   ├── Resources/
│   │   └── Scene/
│   └── main.cpp
├── tests/
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
