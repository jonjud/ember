# EMBER Engine Roadmap

EMBER is developed as a 2D-first engine. Each milestone should produce a runnable, testable result before the next layer is added.

## Phase 1 — Engine Foundation

- [x] Initialize the CMake project
- [x] Integrate SDL3
- [x] Create an application entry point
- [x] Add a window, event loop, and basic 2D render test
- [ ] Verify builds on Windows
- [ ] Add frame timing and a stable update loop
- [ ] Add keyboard and mouse input abstraction
- [ ] Separate the application layer from the engine core

## Phase 2 — 2D Core

- [ ] Renderer abstraction
- [ ] Texture and asset loading
- [ ] Sprite rendering
- [ ] Transform and 2D camera
- [ ] Scene and entity model
- [ ] Collision and Box2D integration
- [ ] Resource lifetime and asset cache

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
- [ ] Logging and diagnostics
- [ ] Profiler
- [ ] User documentation

## Development rules

1. Keep dependencies optional or isolated where practical.
2. Do not add a subsystem before a small use case proves it is needed.
3. Keep editor code separate from runtime code.
4. Prefer small pull requests and buildable commits.
5. Do not claim a platform is supported until it has been built and tested there.
