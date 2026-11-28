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
- [x] Complete structured logging foundation: bounded thread-safe Engine Logger, Editor Log Window, console/file sinks, and Scene Physics/Audio/Scripting diagnostic forwarding with duplicate-suppression tests (Windows CI #303–#315 Success)
- [x] Complete Log Window usability and operations: severity/text filters, per-user log directory, 5 MiB file rotation with one backup, and separate Editor/game-runtime log files (Windows CI #317–#318 Success)

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
- [x] Undo / Redo: bounded serialized Scene snapshot history (100 entries / 16 MiB), in-memory SceneSerializer snapshot APIs, Edit menu and Ctrl+Z/Ctrl+Y/Ctrl+Shift+Z; excludes Play Mode state (Windows CI #46156677787 Debug + Release Success)
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
- [x] Physics Joints (PhysicsWorld API, serializable Revolute/Distance components, Entity-reference Inspector controls, Scene lifecycle integration, automated tests passed Windows CI #191, and final Windows Editor verification user-confirmed passed)
- [x] Animation (SpriteAnimation2D frame playback, Scene lifecycle, serialization, Inspector controls, sprite-sheet viewport preview, Renderer2D source-region API and multi-row frame tests; Windows CI #199 Success; user-confirmed all checks passed)
- [x] Audio implementation and Windows Editor verification (miniaudio runtime, AudioSource2D serialization/Inspector and safe Audio-asset picker, Scene asset resolution/playback, Play Mode runtime controls, safe Scene move/Stop lifetime, Editor Asset Browser preview, edge-case tests; Windows CI #219 Success; final Editor checklist user-confirmed passed)
- [x] UI authoring: Canvas/Panel/Text/Button components, serialization, Inspector, reference-resolution layout and Editor viewport preview (Windows CI #223 Success)
- [x] UI runtime pointer routing: Canvas-aware hit testing, topmost-button selection, pointer capture, hover/press/click state, click event generation and Editor viewport input-capture gating (viewport-aware routing CI #227 Success)
- [x] Deliver UI Button click events to runtime Component callbacks with end-to-end automated coverage (Windows CI #225/#226 Success)
- [x] Guard Scene click dispatch: require an active Entity with an enabled, interactable, valid UIButton2D before invoking callbacks (Windows CI #229/#230 Success)
- [x] Complete Windows Editor UI verification session (checklist: [UI_RUNTIME_VERIFICATION.md](UI_RUNTIME_VERIFICATION.md); user-confirmed passed)
- [x] Runtime Input Action Mapping implementation: named actions, multiple physical-key bindings, edge states, Editor rebind/custom-action UI, project persistence, and Scene-to-Component input forwarding (Windows CI run 46081947029 Success)
- [x] Final Windows Editor verification for Input Action Mapping (combined checklist: [SCRIPTING_RUNTIME_VERIFICATION.md](SCRIPTING_RUNTIME_VERIFICATION.md); user-confirmed passed 2026-11-28)
- [x] Scripting foundation: language-neutral ScriptRuntime host contract and serializable ScriptComponent
- [x] Scene scripting lifecycle integration: runtime initialization, per-Entity ScriptComponent instance ownership, OnCreate/OnUpdate/OnDestroy, fault isolation and diagnostics (Windows CI run 46091385853 Success)
- [x] Script asset type/import pipeline and Editor assignment workflow (language-neutral Script type, Editor assignment, `.lua` language tagging; CI runs 46092542143 and 46097066791 Success)
- [x] Evaluate embedded scripting runtime candidates; select Lua 5.5.1 as the first optional backend while preserving the language-neutral host contract
- [x] Implement Lua 5.5.1 backend: per-instance VM state, lifecycle callbacks, safe asset resolution and diagnostics (Windows CI run 46097066791 Success)
- [x] Expose minimal Entity/Transform/Input API to Lua without leaking Lua types into Engine interfaces (CI run 46097066791 Success)
- [x] Integrate Lua backend with Editor Play Mode and add lifecycle/error/isolation/sandbox tests (instruction and memory budgets, protected initialization, pcall/xpcall budget enforcement, text-only loading; CI run 46097066791 Success)
- [x] Add Script Diagnostics window with Entity-aware diagnostics and scrollable message list (CI run 46101206269 Success)
- [x] Add built-in Script Source Editor for text Script assets, with 64 KiB limit, explicit save/discard, dirty-state protection, external-file conflict detection, failure-safe reload, atomic save replacement, bounded external-file conflict reads and Play Mode write guard (Windows CI runs 46105718716 and 46110556033 Success)
- [x] Combined Windows Editor verification for Input Action Mapping and Lua scripting workflow, including diagnostics, validation, save/reload, dirty-state and exit protection (user-confirmed passed 2026-11-28; Windows CI #288 Success)

## Phase 5 — Packaging and Quality
- [x] Windows game export / runtime executable: standalone `ember_runtime` target, safe Windows export service, runtime DLL staging and manifest, Editor `File > Export Windows Game...` workflow (Windows CI #320–#329 Success; manual export/run checklist remains pending)
- [x] Built-in Project Templates: Blank 2D and 2D Starter, atomic creation, default movement bindings and Editor New Project workflow (Windows CI #332–#336 Success)
- [x] Automated tests executed in CI
- [ ] Profiler
- [ ] User documentation and samples

## Phase 6 — Extensibility and Studio Customization (planned)
These are strategic goals after the core engine, scripting workflow and packaging are stable. They are intentionally not prerequisites for the current scripting milestone.

- [ ] Theme system: Dark, Light and Custom themes; consistent tokens for Editor panels, text, accents, borders, Inspector, Hierarchy and viewport
- [ ] Theme persistence and exchange: project/user theme selection plus import/export of documented theme files
- [ ] Studio themes and branding: studio-defined defaults and controlled Editor appearance customization
- [ ] Multi-language scripting backends: add optional languages behind the language-neutral ScriptRuntime API; C++20 remains the primary engine/native language
- [ ] Bundled scripting runtime distribution: supported language runtimes are packaged/managed by EMBER so end users do not install separate Extensions or runtimes
- [ ] Extension API / SDK: stable, versioned public interfaces for third-party modules, with compatibility and lifecycle rules
- [ ] Custom Engine Distribution: documented SDK/build workflow allowing studios to assemble branded EMBER-based engine distributions with selected modules and workflows

### Extensibility design constraints
- Preserve a small, stable C++ engine core and keep Editor-only systems out of runtime libraries.
- Keep scripting VM details private behind ScriptRuntime; language backends are optional and independently testable.
- Define API/ABI versioning, module compatibility, licensing, platform support and packaging before promising third-party binary extensions.
- Treat studio-specific engine distributions as a long-term capability, not a requirement for the first usable EMBER release.

## Scripting language policy
- C++20 is EMBER's primary and first-class implementation language for the Engine, Editor and native game code.
- Keep the Engine's public scripting host API language-neutral; never expose VM-specific types or pointers across it.
- Additional gameplay scripting languages may be supported through optional embedded runtimes. Their required runtime must be bundled/managed by EMBER so game users do not install separate Extensions or language runtimes.
- Prefer a built-in Editor workflow for selecting and using supported languages; extensions are not the required distribution mechanism for language support.
- A C++ development toolchain is still required to compile native C++ game code. The no-extra-install goal applies to end-user runtime setup and optional embedded scripting languages, not to removing the compiler requirement for C++ development.
- Do not select or add a concrete embedded language/runtime until its integration cost, licensing, platform support, debugging, packaging and API needs have been reviewed.

## Development rules
1. Keep third-party dependencies behind narrow engine-facing interfaces.
2. Keep editor code separate from runtime code.
3. Prefer small, buildable milestones.
4. Distinguish CI compilation/tests from interactive runtime verification.
5. Update [PROJECT_STATUS.md](PROJECT_STATUS.md) whenever the active milestone, current HEAD or CI status changes.

## Next focused milestone
- Windows Runtime/Editor workflow hardening is complete through commit `cfb6058` and Windows CI run `46152882245` (Debug and Release Success): UTF-8 project arguments, runtime project discovery, static-CRT Release export support, Editor Open Project, popup scope fixes, Runtime UI/button routing and SpriteAnimation frame selection, physics body-mode branching, Release-safe test checks/CRT setup and FetchContent caching.
- Editor Undo / Redo is implemented through commit `f552c93` and Windows CI run `46156677787` (Debug and Release Success): canonical in-memory Scene serialization, bounded history (100 entries / 16 MiB), Edit menu and Ctrl+Z/Ctrl+Y/Ctrl+Shift+Z; Play Mode state is excluded. Next Editor milestone: multi-selection and hierarchy organization. Combined manual Windows verification of project templates and exported-game launch remains pending and must not be marked complete until user confirmation. C++20 remains the primary language for Engine and Editor; Lua is an optional gameplay scripting language.
- After the scripting and packaging foundations are stable, begin the planned Phase 6 extensibility work: Theme System, additional language backends, Extension API/SDK and studio-custom Engine distributions.
