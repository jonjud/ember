# EMBER Engine Roadmap

> Live status, latest commit/CI, active milestone and handoff: [PROJECT_STATUS.md](PROJECT_STATUS.md).  
> Physics design decisions: [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md).

EMBER is a 2D-first C++20 engine. Each milestone should produce a buildable, testable vertical slice.

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
- [x] Transform2D and Camera2D
- [x] Scene and Entity model with stable IDs
- [x] RenderComponent2D
- [x] AssetDirectory, AssetManager, AssetRegistry and import metadata
- [x] Versioned project and scene serialization/migration
- [x] Scene dirty-state tracking and Save / Discard / Cancel prompts
- [x] Pin Box2D v3.2.0 as private physics backend
- [x] PhysicsWorld facade, body/collider API and automated tests
- [ ] Scene-owned Box2D simulation and collision integration

## Phase 3 — Editor
- [x] Dear ImGui editor integration isolated from ember_engine
- [x] Main editor layout and menu
- [x] Scene Hierarchy and Inspector
- [x] Asset Browser and 2D viewport
- [x] Entity picking, drag, Move/Rotate/Scale gizmos
- [x] Transform snapping and Local / World orientation
- [x] Play / Stop using isolated Scene clone
- [x] Play Mode movement with WASD / arrow keys
- [x] Duplicate/delete entities and dirty-state prompts
- [x] Persistent Input Settings and rebinding
- [x] Component Inspector: display, add, remove, edit registered components
- [x] Typed Inspector widgets: bool, int, float, string, Vector2, Enumeration
- [ ] Undo / Redo
- [ ] Multi-selection and hierarchy organization
- [ ] Interactive Editor/runtime verification

## Phase 4 — Game Systems
- [x] General component lifecycle and cloning API
- [x] Stable component type registry and factories
- [x] Generic component serialization and schema migrations
- [x] PhysicsBody2D, BoxCollider2D and CircleCollider2D configuration components
- [x] Physics component registration, serialization and Inspector properties
- [ ] Scene PhysicsWorld ownership and lifecycle integration
- [ ] Fixed timestep and transform synchronization
- [ ] Physics collision integration tests and debug visualization
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
1. Keep third-party dependencies behind narrow engine-facing interfaces.
2. Keep editor code separate from runtime code.
3. Prefer small, buildable milestones.
4. Distinguish CI compilation/tests from interactive runtime verification.
5. Update [PROJECT_STATUS.md](PROJECT_STATUS.md) whenever the active milestone, current HEAD or CI status changes.
