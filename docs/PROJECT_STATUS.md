# EMBER Engine — Living Project Status

> เอกสารนี้เป็น Single Source of Truth สำหรับสถานะงานและการส่งต่องานระหว่างบทสนทนา  
> ดูภาพรวมที่ [ROADMAP.md](ROADMAP.md), โครงสร้างที่ [ARCHITECTURE.md](ARCHITECTURE.md), และ Physics decisions ที่ [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md)

## 1. สถานะปัจจุบัน

| รายการ | ค่า |
|---|---|
| Repository | [jonjud/ember](https://github.com/jonjud/ember) |
| Active branch | `develop` |
| Stable branch | `main` |
| Last code commit | [`92afc0c`](https://github.com/jonjud/ember/commit/92afc0c8e6968a01a7bad2805f9229947d8209ba) — SDL3 base path type fix for Editor export workflow |
| Latest repository HEAD | [`92afc0c`](https://github.com/jonjud/ember/commit/92afc0c8e6968a01a7bad2805f9229947d8209ba) — SDL3 base path type fix for Editor export workflow |
| Last verified code CI | [Windows CI #329](https://github.com/jonjud/ember/actions/runs/46146571294) — **Success**; Windows build and all engine tests passed |
| Latest CI | [Windows CI #329](https://github.com/jonjud/ember/actions/runs/46146571294) — **Success** |
| Latest documentation CI | Windows CI #288 — **Success** |
| Last completed milestone | Scripting + Input Action Mapping: Lua authoring/Play Mode, diagnostics, input rebinding, validation/save/reload and unsaved-work protection; combined Windows Editor checklist user-confirmed passed 2026-11-28 and CI #288 Success |
| Active milestone | Project Templates. Windows game export/runtime implementation is complete through CI #329; manual export-and-run verification remains pending. |
| Physics backend | Box2D v3.2.0, pinned via FetchContent; wrapped by EMBER-owned PhysicsWorld API |

> CI Success ยืนยัน Windows build และ automated tests; interactive verification เป็นผลทดสอบแยกต่างหากจากผู้ใช้

## 2. สิ่งที่เสร็จแล้ว

### Structured Logging & Diagnostics — foundation complete (Windows CI passed)
- [x] Add bounded, thread-safe Engine Logger API with severity filtering, sequence/timestamp/thread metadata, snapshots, clear and concurrent-writer tests (Windows CI #303 Success)
- [x] Add Editor Log Window with severity/category/message display, severity colors, clear action and bounded-entry counter; log key Editor lifecycle, Play Mode, Scene save and Asset scan events (Windows CI #304 Success)
- [x] Add ConsoleLogSink and FileLogSink; sink callbacks execute outside Logger history lock and sink exceptions are isolated (Windows CI #306 and follow-up #314 Success)
- [x] Forward Scene Physics, Audio and Scripting diagnostics into Engine Logger with severity/category metadata and duplicate-forward suppression (Windows CI #315 Success)
- [x] Add isolated tests for history, recording sink, file sink, zero-capacity sink and concurrent writers (Windows CI #303–#315 Success)
- [x] Improve Log Window usability with severity/text filtering (Windows CI #317 Success)
- [x] Store Editor/runtime logs in the SDL per-user preference directory; rotate at 5 MiB and retain one `.1` backup (Windows CI #318 Success)
- [x] Separate Editor and game-runtime Logger/file outputs; Play Mode Scene diagnostics go to the runtime logger (Windows CI #318 Success)
- [x] Add automated file-rotation coverage (Windows CI #318 Success)


### Windows game export / runtime packaging — implementation complete (Windows CI passed; manual verification pending)
- [x] Add standalone `ember_runtime` target without Editor/ImGui dependencies; load project config, assets, startup Scene, runtime components, input, Renderer2D, audio and Lua scripts (Windows CI #320 Success)
- [x] Add safe Windows export service with project-root/output validation, asset and startup-scene copying, manifest generation and automated safety tests (Windows CI #321–#322 Success)
- [x] Stage and package runtime DLL dependencies beside the executable; record dependency names and startup Scene in `ember-export.json` (Windows CI #323–#327 Success)
- [x] Add Editor `File > Export Windows Game...` workflow with saved-scene/Play Mode guards and export diagnostics (Windows CI #328–#329 Success)
- [ ] Manual Windows verification: export a real project, inspect output folder, launch exported game on a machine/session without Editor, and verify assets/audio/Lua runtime behavior

### Scripting authoring
- [x] Lua Script syntax preflight with actionable line/column diagnostics and validation-state reset on open/reload/discard (Windows CI run 46105064345 Success)
- [x] Lua 5.5.1 gameplay scripting backend; C++20 remains the primary Engine/Editor implementation language
- [x] Script Diagnostics window with Entity-aware fault messages (Windows CI #269 Success)
- [x] Initial Script Source Editor workflow (Windows CI #270 Success)

### Engine / Editor foundations
- [x] Built-in Script Source Editor for text Script assets: open from Asset Browser, edit/save, 64 KiB cap, explicit discard and unsaved-close protection (Windows CI run 46102606726 Success)
- [x] Detect external source changes before save and reload only when the Editor buffer is clean (Windows CI run 46104170651 Success)
- [x] Failure-safe reload and atomic save replacement; failed reads preserve the current buffer and failed replacement preserves the original file (Windows CI run 46105718716 Success)
- [x] Bound external Script Source conflict reads to 64 KiB and detect file growth during reads before comparing snapshots (Windows CI run 46110556033 Success)
- [x] Protect unsaved Script and Scene changes on application exit, including the window-close request (Windows CI run 46108460101 Success)
- [x] CMake/C++20, SDL3 application loop, timing and input foundation
- [x] Renderer2D, Transform2D, Camera2D, Scene/Entity
- [x] AssetDirectory, AssetManager, AssetRegistry, texture cache and import metadata
- [x] Versioned project/scene/asset serialization and migration
- [x] Editor hierarchy, Inspector, viewport, gizmos, Play/Stop with isolated Scene clone
- [x] Generic Component lifecycle, clone, registry, factory, serialization and schema migrations
- [x] Add/remove registered Components through Editor UI
- [x] Typed Inspector property descriptors and widgets: bool, int, float, string, Vector2, Enumeration, EntityReference

### UI System — complete (Windows Editor verified)
- [x] Add serializable UICanvas2D and UIPanel2D components with validation and Clone support
- [x] Register UI components and expose reference resolution, geometry, color and enabled properties in Inspector
- [x] Add reference-resolution layout with aspect-preserving scale and letterboxing
- [x] Add automated tests for serialization, cloning, validation and viewport layout (Windows CI #221 Success)
- [x] Render enabled UI panels as a screen-space overlay in the Editor viewport
- [x] Add serializable UI Text and Button components with validation, cloning and Inspector properties
- [x] Render UI Text and Button previews in the Editor viewport
- [x] Add runtime UI pointer routing, Canvas-aware hit testing, transient hover/press/click state and Editor viewport input-capture gating
- [x] Dispatch UI Button click events to runtime Component callbacks; end-to-end automated coverage verifies pointer release, outside-release cancellation, non-Button entities and disabled/non-interactable Buttons
- [x] Complete Windows Editor UI verification (user-confirmed passed)

### Physics — completed slices
- [x] Pin Box2D v3.2.0 and isolate it behind private engine implementation
- [x] Define units, coordinate conversion, ownership and lifecycle rules in [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md)
- [x] Implement engine-owned PhysicsWorld RAII facade and body/collider API
- [x] Automated PhysicsWorld tests
- [x] Add serializable configuration components: PhysicsBody2D, BoxCollider2D, CircleCollider2D
- [x] Add ColliderSet2D with stable ColliderId generation, lookup/removal, validation and clone tests (CI #135)
- [x] Add canonical ColliderSet2D serialization and legacy Box/Circle Scene migration (CI #151)
- [x] Register physics component types, serializers/deserializers and Inspector properties
- [x] Add Vector2 Inspector controls and enum dropdown support
- [x] Scene owns PhysicsWorld during runtime and builds bodies/colliders from entity components
- [x] Fixed 1/60 s simulation step with bounded accumulator, frame-delta clamp and substep cap
- [x] Dynamic body -> Entity Transform2D synchronization; static/kinematic transform -> physics synchronization
- [x] Scene Stop tears down runtime physics; runtime handles remain transient and are not cloned/serialized
- [x] Scene physics tests: lifecycle, falling/collision, invalid collider configurations, NaN delta, stop/clone behavior
- [x] Link PhysicsWorld and Box2D into all Scene-based test targets
- [x] Validate physics body/collider numeric configuration and keep failed collider creation atomic
- [x] Test disabled bodies and static/kinematic transform authority
- [x] Add Editor viewport Physics debug overlay for box/circle collider outlines (CI #113)
- [x] Add engine-owned PhysicsWorld Begin/End contact events mapped to Scene EntityIds
- [x] Add closest raycast and AABB overlap queries with engine-owned query filters/results and Scene EntityId mapping
- [x] Create multiple ColliderSet2D Box/Circle shapes on one PhysicsBody; preserve per-shape query filters and Entity-level query deduplication (CI #155)
- [x] Aggregate overlapping shape-pair contact transitions into one Entity-pair Begin/End event (CI #164)
- [x] Add Inspector list editing for ColliderSet2D: Add Box/Circle, duplicate, remove, edit geometry/material/filter properties (CI #169)
- [x] Extend Physics Debug Overlay to draw every ColliderSet2D Box/Circle entry, retaining sensor/solid colors and legacy fallback (CI #170)
- [x] Verify query category/mask filtering, sensor inclusion and Scene forwarding
- [x] Verify Physics Debug Overlay and Play Mode interactively on Windows for the original single-collider slice (user-confirmed; CI #132)
- [x] Verify multi-collider Inspector, overlay, persistence/migration and Play Mode interactively on Windows (user-confirmed; checklist: [PHYSICS_RUNTIME_VERIFICATION.md](PHYSICS_RUNTIME_VERIFICATION.md))
- [x] Collect contact events after each fixed simulation step; Scene exposes the latest Update's events until next Update
- [x] Wake touching dynamic bodies when a transform-driven static/kinematic body moves so contact separation can be processed
- [x] Add engine-owned PhysicsJointHandle and Revolute/Distance joint descriptors with create/destroy/count API; keep Box2D joint handles private
- [x] Add joint validation and lifetime tests: invalid same-body/length/limits, explicit destruction, and automatic cleanup when a body is destroyed
- [x] Physics Joints: serializable Revolute/Distance components, Entity references, Scene lifecycle, Inspector picker, serialization and edge-case tests (Windows CI #191; final Windows Editor verification user-confirmed passed)
- [x] SpriteAnimation2D: frame playback, Scene component updates, serialization, Inspector controls, sprite-sheet viewport preview, source-region rendering API and multi-row frame tests (Windows CI #199 Success; user-confirmed all checks passed)
- [x] Add serializable RevoluteJoint2D and DistanceJoint2D authoring components with EntityId references and cloned configuration
- [x] Create Scene joints after all eligible bodies; validate self/missing/disabled/invalid references and convert pixel anchors/length to physics meters
- [x] Expose Connected Entity selection and joint settings in the Editor Inspector
- [x] Add component-registry, Scene lifecycle, and full Scene serialization round-trip tests for joint references

## 3. Physics decisions to preserve
- Box2D v3.2.0 is private; Box2D types must not leak into public Scene/Entity/Component/Editor APIs.
- A running Scene owns its PhysicsWorld; Editor Play Mode operates on an isolated Scene clone.
- Physics units are meters; conversion is 100 pixels = 1 meter.
- EMBER world uses positive Y downward; default gravity points positive Y.
- Entity Transform2D position maps to body origin; rotation is radians.
- Collider geometry is explicit, never inferred from sprite/render size.
- Physics handles are runtime-only and never serialized or cloned as raw backend IDs.
- Fixed timestep is 1/60 s with bounded accumulator and substeps.
- ColliderSet2D is the canonical multi-shape configuration: one PhysicsBody2D owns an ordered set of Box/Circle shapes. Legacy BoxCollider2D/CircleCollider2D inputs migrate to one-entry sets during Scene loading; conflicting combinations are rejected.
- Collider without PhysicsBody2D is skipped with a diagnostic.
- Dynamic bodies are physics-authoritative; static and kinematic bodies are transform-authoritative.

## 4. Current limitations / next milestone

### Runtime Input Action Mapping — complete and Windows Editor-verified
- [x] Named action bindings with multiple SDL physical scancodes, duplicate/unknown-key rejection, bind/unbind/clear/rename and default movement map
- [x] Per-frame action state with Down / Pressed / Released edges
- [x] Editor Input Settings: rebind, add secondary key, create/rename/delete custom actions, conflict prevention and reset-to-default
- [x] Persist input bindings in the project configuration with schema-v1 compatibility and validation
- [x] Feed the current InputActionState into running Scene Components; Editor Play Mode uses named movement actions
- [x] Automated coverage: InputMap transitions, project configuration persistence, and Scene-to-Component input forwarding
- [x] Windows CI run [46081947029](https://github.com/jonjud/ember/actions/runs/46081947029) — Success
- [x] Final interactive Windows Editor verification: combined checklist [SCRIPTING_RUNTIME_VERIFICATION.md](SCRIPTING_RUNTIME_VERIFICATION.md), user-confirmed passed 2026-11-28

**UI System v1 is complete and Windows Editor-verified.** UI Canvas/Panel/Text/Button authoring, serialization, Inspector properties, aspect-preserving layout, Editor viewport preview, viewport-aware pointer routing, pointer capture, click generation and guarded runtime Component callback dispatch are implemented. Windows CI #225–#231 passed, including end-to-end callback coverage and rejection of invalid click targets. The user confirmed the final Windows Editor verification passed.

**Multiple Colliders per Entity is complete.** Model, serialization/migration, runtime/query filtering, Entity-pair contact aggregation, Inspector editing and multi-shape Debug Overlay passed their respective Windows CI runs (#135, #151, #155, #164, #169, #170). The user confirmed the Windows Editor checklist passed on 2026-11-27, including Editor launch/layout, Entity and Transform editing, duplicate/delete, PhysicsBody2D Inspector add/edit/remove, ColliderSet2D editing, Physics Debug Overlay, save/reopen persistence, and Play/Stop behavior.

**Audio Runtime is complete and Windows Editor-verified.** The miniaudio 0.11.26 runtime facade, Application/Editor audio lifecycle, device-less generated-WAV tests, serializable `AudioSource2D`, typed Inspector properties and safe Audio-asset picker, Scene asset-resolution/playback lifecycle, runtime Play/Stop controls, safe Scene move/Stop lifetime, and Editor Asset Browser preview are implemented. Windows CI #219 passed. Automated coverage includes unavailable/uninitialized devices, missing dependencies/assets, inactive/disabled sources, invalid volume, wrong asset types and hand-edited traversal metadata. The final Windows Editor checklist in [AUDIO_RUNTIME_VERIFICATION.md](AUDIO_RUNTIME_VERIFICATION.md) was user-confirmed passed. Sprite Animation is complete (CI #199 Success and user-confirmed checks passed). Physics Joints automated tests passed CI #191 and final Windows Editor verification was user-confirmed passed.

### Scripting language policy
- [x] C++20 is the primary and first-class language for Engine, Editor and native game code
- [x] Additional languages may be supported through optional embedded runtimes bundled/managed by EMBER; end users should not need separate Extension or runtime installation
- [x] Keep the host API language-neutral and VM implementation details private
- [x] Evaluate Lua, Wren, ChaiScript and AngelScript; select Lua 5.5.1 as first optional backend after reviewing embedding model, portability, license and C++ integration
- [ ] C++ native game-code build workflow and optional language workflows

### Scripting foundation and Lua backend — implementation complete (Windows CI passed)
- [x] Language-neutral ScriptRuntime interface and opaque ScriptInstanceHandle; VM pointers remain private
- [x] Serializable ScriptComponent, ComponentRegistry registration and typed Inspector assignment
- [x] Scene lifecycle integration: runtime initialization, per-Entity instance ownership, OnCreate/OnUpdate/OnDestroy, fault isolation and diagnostics (CI run 46091385853 Success)
- [x] Script Asset import, Asset Browser listing/filter and Editor assignment workflow (CI run 46092542143 Success)
- [x] Language-aware Script metadata: optional `script_language` identifier; legacy sidecars remain compatible; `.lua` imports are tagged `lua`; non-Script assets cannot declare a language
- [x] Lua 5.5.1 static embedded backend with per-instance VM state and Editor Play Mode integration
- [x] Minimal Entity/Transform/Input API without exposing Lua types through public Engine interfaces
- [x] Runtime limits and sandbox hardening: memory/instruction budgets, protected initialization, budget enforcement through pcall/xpcall, text-only load and no string.dump
- [x] Automated lifecycle, transform/input, syntax-error, isolation, language-mismatch and sandbox tests (Windows CI run 46097066791 Success)
- [x] Combined Windows Editor verification for Lua Play Mode and Input Action Mapping, including Script Diagnostics, source validation/save/reload and unsaved-work protection (user-confirmed passed 2026-11-28; Windows CI #288 Success)

See [SCRIPTING_DESIGN.md](SCRIPTING_DESIGN.md) for the C++-first language policy, candidate evaluation, selected Lua version, backend boundaries and deferred decisions.

## 5. Development rules
1. Work on `develop` only. Never merge to `main` without explicit approval.
2. Inspect latest `develop` HEAD before every implementation change.
3. Keep milestones small, buildable and testable.
4. After a milestone, update this file and [ROADMAP.md](ROADMAP.md); update [ARCHITECTURE.md](ARCHITECTURE.md) when behavior/ownership changes.
5. Never report CI passed until the workflow is completed with `conclusion: success`.
6. Distinguish CI build/tests from interactive runtime verification.
7. If CI fails, inspect logs, fix the root cause, and wait for a successful rerun before closing the milestone.
8. Defer manual Windows Editor testing until the active milestone is complete; run automated CI throughout development.

## 6. Recent milestone log

| 2026-11-28 | [Engine Logger API, `2d05980`, CI #303](https://github.com/jonjud/ember/actions/runs/46117251072) | Success; bounded thread-safe Logger and automated tests |
| 2026-11-28 | [Editor Log Window, `a1533a7`, CI #304](https://github.com/jonjud/ember/actions/runs/46117795488) | Success; Windows build and engine tests passed |
| 2026-11-28 | [Logger sinks and Scene diagnostics, `0057a6b`, CI #315](https://github.com/jonjud/ember/actions/runs/46130841060) | Success; Console/File sinks and Physics/Audio/Scripting diagnostic forwarding verified |
| 2026-11-28 | [Log filters, `2ddf473`, CI #317](https://github.com/jonjud/ember/actions/runs/46131695550) | Success; severity and text filtering in the Editor Log Window |
| 2026-11-28 | [Log rotation and runtime separation, `25b8fc2`, CI #318](https://github.com/jonjud/ember/actions/runs/46132259115) | Success; per-user logs, bounded rotation and separate Editor/runtime output |
| 2026-11-28 | [Standalone runtime, exporter, DLL manifest and Editor export UI, CI #320–#329](https://github.com/jonjud/ember/actions/runs/46146571294) | Success; Windows build and all automated engine/exporter tests passed; manual export-and-run verification remains pending |


| Date | Commit / CI | Result |
|---|---|---|
| 2026-11-27 | [PhysicsWorld wrapper, CI #100](https://github.com/jonjud/ember/actions/runs/45925546626) | Success |
| 2026-11-27 | [Physics component config types, CI #101](https://github.com/jonjud/ember/actions/runs/45929634947) | Success |
| 2026-11-27 | [Physics registration/Inspector, CI #103](https://github.com/jonjud/ember/actions/runs/45933429501) | Success |
| 2026-11-27 | [Scene Physics Runtime Integration, CI #109](https://github.com/jonjud/ember/actions/runs/45935434409) | Success |
| 2026-11-27 | [Physics Debug Overlay, CI #113](https://github.com/jonjud/ember/actions/runs/45937028116) | Success |
| 2026-11-27 | [Scene Contact Event API, CI run 45939999840](https://github.com/jonjud/ember/actions/runs/45939999840) | Success |
| 2026-11-27 | [Raycast and Overlap Query API, CI run 45941862568](https://github.com/jonjud/ember/actions/runs/45941862568) | Success |
| 2026-11-27 | [Query filter and sensor coverage, CI run 45945132511](https://github.com/jonjud/ember/actions/runs/45945132511) | Success |
| 2026-11-27 | [Multi-collider Inspector editing, CI #169](https://github.com/jonjud/ember/actions/runs/45959345669) | Success |
| 2026-11-27 | [Multi-collider Physics Debug Overlay, CI #174](https://github.com/jonjud/ember/actions/runs/45961198358) | Success |
| 2026-11-27 | [Multi-collider docs and verification checklist, CI #175–#178](https://github.com/jonjud/ember/actions/runs/45962148407) | Success |
| 2026-11-27 | [Physics Joint API and lifetime tests, `d8a7061`](https://github.com/jonjud/ember/commit/d8a7061143ea6c1246dd5573713ae1ae1f7db9f5) | Committed; CI status not yet confirmed |
| 2026-11-27 | Windows Editor manual verification | User-confirmed PASS: Editor launch/layout, Entity/Transform, duplicate/delete, PhysicsBody2D Inspector, ColliderSet2D, Debug Overlay, persistence, Play/Stop |
| 2026-11-27 | [Audio runtime controls and edge-case tests, `a5b23a3`, CI #219](https://github.com/jonjud/ember/actions/runs/46003199338) | Success; final Editor verification checklist prepared |
| 2026-11-27 | [UI Button click dispatch, `9e2c969`, CI #225](https://github.com/jonjud/ember/actions/runs/46011291135) | Success; runtime Component callback dispatch and initial Editor verification checklist prepared |
| 2026-11-27 | [UI callback integration tests, `0320a08`, CI #226](https://github.com/jonjud/ember/actions/runs/46013967857) | Success |
| 2026-11-27 | [Viewport-aware UI input, `600f407`, CI #227](https://github.com/jonjud/ember/actions/runs/46014213385) | Success |
| 2026-11-27 | [UI status/roadmap refresh, CI #228](https://github.com/jonjud/ember/actions/runs/46014412490) | Success |
| 2026-11-27 | [Guard UI click dispatch, `79c6ab9`, CI #229](https://github.com/jonjud/ember/actions/runs/46015014246) | Success |
| 2026-11-27 | [Test non-Button click rejection, `663d9df`, CI #230](https://github.com/jonjud/ember/actions/runs/46015090528) | Success |
| 2026-11-27 | [Docs refresh, `10ab998` and `4a80448`, CI #232/#233](https://github.com/jonjud/ember/actions/runs/46018916409) | Success |
| 2026-11-27 | Combined Windows Editor verification: UI System v1, Audio Runtime, Physics Joints | User-confirmed PASS |
| 2026-11-28 | [SDL3 runtime staging fix, `e92fe90`, CI run 46081947029](https://github.com/jonjud/ember/actions/runs/46081947029) | Success; all build/tests passed; Input Action Mapping Editor verification checklist prepared |
| 2026-11-28 | [Scene scripting lifecycle + SDL3 runtime staging, `25f613d`, CI run 46091385853](https://github.com/jonjud/ember/actions/runs/46091385853) | Success; Scene script lifecycle automated tests passed |
| 2026-11-28 | [Language-neutral Script Asset import and Editor assignment, `cd36dd1`, CI run 46092542143](https://github.com/jonjud/ember/actions/runs/46092542143) | Success; Script metadata/import tests and Editor build passed |
| 2026-11-28 | [Lua backend + language-tagged Script Assets, `41f5ac3`, CI run 46097066791](https://github.com/jonjud/ember/actions/runs/46097066791) | Success; Lua runtime, Editor Play Mode integration, script language tagging and automated sandbox/lifecycle tests passed |
| 2026-11-28 | [Script Source Editor external-change guard, `1089bd1`, CI run 46104170651](https://github.com/jonjud/ember/actions/runs/46104170651) | Success; external edits are detected before save, clean buffers can safely reload from disk, and Editor build/tests passed |
| 2026-11-28 | [Failure-safe Script Source Editor file I/O, `c6f3c54`, CI run 46105718716](https://github.com/jonjud/ember/actions/runs/46105718716) | Success; reload preserves the in-memory buffer on read failure and save uses atomic replacement |
| 2026-11-28 | [Bound external Script Source conflict reads, `3416971`, CI #285](https://github.com/jonjud/ember/actions/runs/46110556033) | Success; conflict comparison is capped at 64 KiB and detects file growth during the read |

---

**Handoff:** Scripting + Input Action Mapping and Structured Logging & Diagnostics are complete. Windows game export/runtime implementation is complete through CI #329: standalone runtime, safe project exporter, staged DLL dependencies, export manifest and Editor export UI. Next: Project Templates. After implementation, perform the combined manual Windows Editor/exported-game verification (export a sample project and run it outside the Editor). Keep Editor-only code out of `ember_engine`. C++20 remains the primary Engine/Editor/native language. Long-term direction remains in Phase 6 of [ROADMAP.md](ROADMAP.md): Theme System, additional language backends, Extension API/SDK and studio-custom Engine distributions. Continue on `develop`; do not merge to `main`.
