# EMBER Engine

A lightweight, 2D-first game engine written in modern C++.

EMBER is being built from the ground up. The project currently focuses on a modular desktop foundation: application lifecycle, frame timing, keyboard input, and a simple movement/rendering test.

## Project status

**Early development — Phase 1: Engine Foundation**

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
- Press Escape to quit.

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
├── docs/
│   ├── ARCHITECTURE.md
│   └── ROADMAP.md
├── src/
│   ├── Engine/
│   │   ├── Core/
│   │   └── Platform/
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
