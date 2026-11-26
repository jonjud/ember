# EMBER Engine Roadmap

EMBER is developed as a 2D-first engine. Each milestone should produce a runnable, testable result before the next layer is added.

## Phase 1 — Engine Foundation

- [x] Initialize the CMake project
- [x] Integrate SDL3
- [x] Create an application entry point
- [x] Create a window and event loop
- [x] Extract the application lifecycle into an engine module
- [x] Add high-resolution frame timing
- [x] Add a keyboard input adapter
- [x] Add a frame-rate-independent movement test
- [ ] Verify builds and runtime behavior on Windows
- [ ] Add logging and diagnostics

## Phase 2 — 2D Core

- [x] Renderer2D abstraction
- [x] BMP texture loading
- [x] Texture handle lifetime management
- [x] Sprite drawing API
- [ ] PNG/JPEG asset loading
- [x] Basic Transform2D position and scale
- [x] Camera2D world-to-screen conversion and zoom
- [x] Transform rotation and camera rotation
- [x] Camera-aware transformed rectangle and sprite drawing
- [ ] Scene and entity model
- [ ] Collision and Box2D integration
- [ ] Resource cache and asset pipeline

## Phase 3 — Editor

- [ ] Dear ImGui integration
- [ ] Main editor layout
- [ ] Scene hierarchy
- [ ] Inspector
- [ ] Asset browser
- [ ] 2D viewport
- [ ] Play / stop workflow

## Phase 4 — Game Systems

- [ ] Animation
- [ ] Audio
- [ ] UI system
- [ ] Input mapping
- [ ] Save and load scenes
- [ ] Scripting interface

## Phase 5 — Packaging and Quality

- [ ] Windows game export
- [ ] Project templates
- [ ] Automated tests
- [ ] Profiler
- [ ] User documentation

## Development rules

1. Keep dependencies optional or isolated where practical.
2. Do not add a subsystem before a small use case proves it is needed.
3. Keep editor code separate from runtime code.
4. Prefer small, buildable milestones.
5. Do not claim a platform is supported until it has been built and tested there.
