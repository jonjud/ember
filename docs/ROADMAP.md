# EMBER Engine Roadmap

EMBER is a 2D-first engine. Each milestone should produce a buildable, testable result before the next layer is added.

## Phase 1 — Engine Foundation

- [x] Initialize CMake project and integrate SDL3
- [x] Application entry point, window, and event loop
- [x] High-resolution frame timing
- [x] Keyboard input adapter
- [x] Frame-rate-independent movement proof of concept
- [x] Windows CI build
- [ ] Verify interactive runtime behavior on Windows
- [ ] Add logging and diagnostics

## Phase 2 — 2D Core and Project Assets

- [x] Renderer2D, primitive drawing, and sprite API
- [x] BMP, PNG, and JPEG texture loading
- [x] Texture handles and reference-counted texture cache
- [x] Transform2D and Camera2D (position, scale, rotation, zoom)
- [x] Scene and Entity model with stable IDs
- [x] RenderComponent2D for rectangles and texture-backed sprites
- [x] AssetDirectory path resolution and traversal checks
- [x] AssetManager and stable-ID asset loading
- [x] Versioned .emberasset metadata and AssetRegistry diagnostics
- [x] Texture import settings (filtering and premultiplied alpha)
- [x] Versioned .emberproject configuration (schema v2)
- [x] Project Settings: name, asset directory, startup scene
- [x] Startup scene loading and Scene Manager workflow
- [x] Versioned .emberscene serialization and migration/validation
- [x] Scene dirty-state tracking with Save / Discard / Cancel prompts
- [ ] Collision detection and Box2D integration

## Phase 3 — Editor

- [x] Dear ImGui editor integration isolated from ember_engine
- [x] Main editor layout and menu
- [x] Scene Hierarchy and Inspector
- [x] Asset Browser with search, type filtering, refresh, and diagnostics
- [x] 2D viewport preview, grid, camera pan/zoom, and entity picking
- [x] Texture previews in viewport
- [x] Drag entities in viewport
- [x] Move, Rotate, and Scale gizmos
- [x] Transform snapping and Local / World orientation
- [x] Play / Stop using an isolated Scene clone
- [x] Play Mode movement with WASD / arrow keys
- [x] Per-entity MovementComponent2D and Inspector controls
- [x] Duplicate and delete entities through UI and shortcuts
- [x] Scene dirty-state prompts before destructive scene operations
- [x] Persistent Input Settings: named actions, rebinding, custom actions, conflict checks, reset defaults
- [ ] Undo / Redo
- [ ] Multi-selection and hierarchy organization
- [ ] Editor usability and runtime integration verification

## Phase 4 — Game Systems

- [x] General component update/lifecycle API (typed components, cloning, Start / Update / Stop)
- [x] Stable component type registry and creation factories
- [x] Generic component serialization through stable type IDs and registered callbacks
- [x] Component Inspector: display, add, remove, and edit registered components
- [x] Typed component property descriptors and generic Inspector widgets (bool, int, float, string)
- [ ] Component schema migration/versioned component payloads
- [ ] Collision and physics integration (planned Box2D backend)
- [ ] Animation
- [ ] Audio (planned miniaudio backend)
- [ ] UI system
- [ ] Runtime input action mapping integration
- [ ] Scripting interface

## Phase 5 — Packaging and Quality

- [ ] Windows game export / runtime executable
- [ ] Project templates
- [x] Automated tests executed in CI
- [ ] Profiler
- [ ] User documentation and samples

## Development rules

1. Keep third-party dependencies isolated behind narrow engine-facing interfaces where practical.
2. Do not add a subsystem before a small use case proves it is needed.
3. Keep editor code separate from runtime code.
4. Prefer small, buildable milestones.
5. Distinguish CI compilation/tests from interactive runtime verification.
