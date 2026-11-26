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
- [x] Verify Windows CI build
- [ ] Verify interactive runtime behavior on Windows
- [ ] Add logging and diagnostics

## Phase 2 — 2D Core

- [x] Renderer2D abstraction
- [x] BMP texture loading
- [x] PNG/JPEG image decoding through SDL3_image
- [x] Texture handle lifetime management
- [x] Sprite drawing API
- [x] Basic Transform2D position and scale
- [x] Camera2D world-to-screen conversion and zoom
- [x] Transform rotation and camera rotation
- [x] Camera-aware transformed rectangle and sprite drawing
- [x] Scene and entity model
- [x] Connect active Scene to the application/render loop
- [x] Add a basic render component for texture-backed sprites
- [x] Add a path-based reference-counted texture cache
- [x] Add an asset root resolver with traversal checks
- [x] Add project-facing AssetManager for validated texture loading
- [x] Versioned asset metadata sidecar format and texture import settings
- [x] Asset metadata project scanning and diagnostics registry
- [x] Resolve and load texture assets through stable IDs in AssetManager
- [x] Apply texture filter and premultiplied-alpha import settings at load time
- [x] Add an asset import settings service API for editor workflows
- [x] Add editor UI for texture filter and premultiplied-alpha settings
- [x] Versioned `.emberproject` configuration with validated relative paths
- [x] Versioned JSON scene serialization with stable entity and asset IDs
- [ ] Collision and Box2D integration

## Phase 3 — Editor

- [x] Dear ImGui integration (editor-only SDL3 platform and renderer backends)
- [x] Main editor layout (menu bar, hierarchy, inspector, asset browser, viewport placeholders)
- [x] Scene hierarchy (create/select entities)
- [x] Inspector (edit entity name, active state, transform, and render properties)
- [x] Asset browser (registry-backed list, type filter, search, selection, refresh, diagnostics)
- [x] 2D viewport preview (scene rectangles, grid, camera pan/zoom, entity picking)
- [x] Registry-backed texture previews in the 2D viewport (filter and premultiplied-alpha settings)
- [x] Direct entity position editing by dragging selected entities in the 2D viewport
- [x] 2D translation gizmo with world-space X/Y axis-constrained dragging
- [x] 2D rotation and scale gizmo tools
- [x] Optional transform snapping for position, rotation, and scale
- [x] Local-space and world-space move/scale gizmo orientation
- [x] Play / stop workflow with an isolated Scene clone
- [x] Basic Play Mode runtime update loop with frame-rate-independent WASD / arrow-key movement
- [x] Per-entity MovementComponent2D with Inspector settings and scene serialization

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
