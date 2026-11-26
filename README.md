# EMBER Engine

A lightweight, 2D-first game engine written in modern C++.

EMBER is being built from the ground up. The first milestone focuses on a reliable desktop foundation: window creation, the main loop, input handling, and a simple 2D renderer.

## Project status

**Early development — Phase 1: Engine Foundation**

See [ROADMAP.md](docs/ROADMAP.md) for the planned milestones.

## Technology

- C++20
- CMake
- SDL3 (windowing, input, and initial 2D rendering)

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
├── src/
│   └── main.cpp
├── .gitignore
├── CMakeLists.txt
└── README.md
```

## Design principles

1. Keep the core small and modular.
2. Prefer understandable code over premature abstraction.
3. Build runnable vertical slices and test each milestone.
4. Keep third-party dependencies isolated.
5. Treat 2D as the first-class target; consider 3D only after the foundation is stable.

## License

License selection is pending. Third-party dependencies retain their own licenses.
