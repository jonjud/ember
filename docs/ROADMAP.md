# EMBER Engine Roadmap

> Live status, latest commit/CI and handoff: [PROJECT_STATUS.md](PROJECT_STATUS.md).  
> Physics design decisions: [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md).

EMBER is a 2D-first C++20 engine. Each milestone should produce a buildable, testable vertical slice.

## Phase 1 — Engine Foundation
- [x] Initialize CMake project and integrate SDL3
- [x] Application entry point, window, and event loop
- [x] High-resolution frame timing
- [x] Keyboard input adapter
- [x] Frame-rate-independent movement proof of concept
- [x] Windows CI build
- [x] Verify interactive runtime behavior on Windows (manual checklist passed)
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
- [x] Scene-owned physics simulation and transform synchronization

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
- [x] Typed Inspector widgets: bool, int, float, string, Vector2, Enumeration, EntityReference
- [x] Physics debug overlay (box/circle collider outlines; Windows CI #113)
- [ ] Undo / Redo
- [ ] Multi-selection and hierarchy organization
- [x] Interactive Editor/runtime verification (Windows manual checklist passed; CI #132)
- [x] Windows Editor smoke test: launch/layout, hierarchy, Inspector, viewport, Entity/Transform editing, duplicate/delete (user-confirmed 2026-11-27)
- [x] Windows Physics Inspector workflow: PhysicsBody2D add/edit/remove and ColliderSet2D editing (user-confirmed 2026-11-27)
- [x] Windows Physics Debug Overlay, scene persistence, and Play/Stop verification (user-confirmed 2026-11-27)

## Phase 4 — Game Systems
- [x] General component lifecycle and cloning API
- [x] Stable component type registry and factories
- [x] Generic component serialization and schema migrations
- [x] PhysicsBody2D, BoxCollider2D and CircleCollider2D configuration components
- [x] Physics component registration, serialization and Inspector properties
- [x] Scene PhysicsWorld ownership and lifecycle integration
- [x] Fixed timestep and transform synchronization
- [x] Scene physics integration tests: lifecycle, simulation, collision, invalid component combinations and teardown
- [x] Numeric physics configuration validation and disabled/static/kinematic body-mode tests
- [x] Collision/contact begin/end event API (engine-owned EntityId results; Windows CI run 45939999840)
- [x] Closest raycast and AABB overlap query API (engine-owned results, query filters and Scene EntityId mapping; Windows CI #121)
- [x] Query-filter category/mask and sensor inclusion tests (Windows CI #126)
- [x] Multiple colliders per entity (model #135; serialization/migration #151; runtime/query #155; contact aggregation #164; Inspector #169; overlay #170; Windows interactive checklist user-confirmed passed)
- [x] Physics Joints (PhysicsWorld API, serializable Revolute/Distance components, Entity-reference Inspector controls, Scene lifecycle integration, and automated tests passed Windows CI #191; final Windows manual checklist is intentionally deferred)
- [x] Animation (SpriteAnimation2D frame playback, Scene lifecycle, serialization, Inspector controls, sprite-sheet viewport preview, Renderer2D source-region API and multi-row frame tests; Windows CI #199 Success; user-confirmed all checks passed)
- [x] Audio implementation (miniaudio runtime, AudioSource2D serialization/Inspector and safe Audio-asset picker, Scene asset resolution/playback, Play Mode runtime controls, safe Scene move/Stop lifetime, Editor Asset Browser preview, and edge-case tests; Windows CI #219 Success). Final Windows Editor verification remains: [AUDIO_RUNTIME_VERIFICATION.md](AUDIO_RUNTIME_VERIFICATION.md)
- [ ] UI system (Canvas/Panel components, reference-resolution layout and Editor viewport runtime preview in progress)
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

## Next focused milestone
- Continue UI System: add UI Text/Button components, layout and interaction routing, then complete Windows Editor verification. Audio implementation and CI #219 are complete; the one-time Windows Audio Editor checklist remains documented in [AUDIO_RUNTIME_VERIFICATION.md](AUDIO_RUNTIME_VERIFICATION.md). Keep miniaudio types private and editor-only code out of `ember_engine`.
