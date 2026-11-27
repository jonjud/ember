# EMBER Engine — Living Project Status

> เอกสารนี้เป็น Single Source of Truth สำหรับสถานะงานและการส่งต่องานระหว่างบทสนทนา  
> ดูภาพรวมที่ [ROADMAP.md](ROADMAP.md), โครงสร้างที่ [ARCHITECTURE.md](ARCHITECTURE.md), และ Physics decisions ที่ [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md)

## 1. สถานะปัจจุบัน

| รายการ | ค่า |
|---|---|
| Repository | [jonjud/ember](https://github.com/jonjud/ember) |
| Active branch | `develop` |
| Stable branch | `main` |
| Last code commit | `663d9df` — Scene validates UI Button state before dispatching callbacks |
| Last verified CI | [Windows CI #227](https://github.com/jonjud/ember/actions/runs/46014213385) — **Success** (viewport-aware UI input and full Windows engine tests) |
| Latest CI | [Windows CI #230](https://github.com/jonjud/ember/actions/runs/46015090528) — **In progress** (`663d9df`; guarded runtime click dispatch) |
| Latest documentation CI | [Windows CI #228](https://github.com/jonjud/ember/actions/runs/46014412490) — **Success** (docs/status update) |
| Last completed milestone | Audio implementation — Windows CI #219 Success; final interactive Editor checklist remains outstanding |
| Active milestone | UI System — Canvas/Panel/Text/Button authoring, serialization, Inspector, reference-resolution layout, viewport preview, pointer routing, runtime Component click dispatch and viewport-aware input capture are implemented. CI #226–#228 passed; click-dispatch hardening is in CI #229/#230. Final Windows Editor verification remains. |
| Physics backend | Box2D v3.2.0, pinned via FetchContent; wrapped by EMBER-owned PhysicsWorld API |

> CI Success ยืนยัน Windows build และ automated tests; interactive verification เป็นผลทดสอบแยกต่างหากจากผู้ใช้

## 2. สิ่งที่เสร็จแล้ว

### Engine / Editor foundations
- [x] CMake/C++20, SDL3 application loop, timing and input foundation
- [x] Renderer2D, Transform2D, Camera2D, Scene/Entity
- [x] AssetDirectory, AssetManager, AssetRegistry, texture cache and import metadata
- [x] Versioned project/scene/asset serialization and migration
- [x] Editor hierarchy, Inspector, viewport, gizmos, Play/Stop with isolated Scene clone
- [x] Generic Component lifecycle, clone, registry, factory, serialization and schema migrations
- [x] Add/remove registered Components through Editor UI
- [x] Typed Inspector property descriptors and widgets: bool, int, float, string, Vector2, Enumeration, EntityReference

### UI System — in progress
- [x] Add serializable UICanvas2D and UIPanel2D components with validation and Clone support
- [x] Register UI components and expose reference resolution, geometry, color and enabled properties in Inspector
- [x] Add reference-resolution layout with aspect-preserving scale and letterboxing
- [x] Add automated tests for serialization, cloning, validation and viewport layout (Windows CI #221 Success)
- [x] Render enabled UI panels as a screen-space overlay in the Editor viewport
- [x] Add serializable UI Text and Button components with validation, cloning and Inspector properties
- [x] Render UI Text and Button previews in the Editor viewport
- [x] Add runtime UI pointer routing, Canvas-aware hit testing, transient hover/press/click state and Editor viewport input-capture gating
- [x] Dispatch UI Button click events to runtime Component callbacks; end-to-end automated coverage verifies pointer release, outside-release cancellation, non-Button entities and disabled/non-interactable Buttons
- [ ] Complete Windows Editor UI verification

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
- [x] Physics Joints: serializable Revolute/Distance components, Entity references, Scene lifecycle, Inspector picker, serialization and edge-case tests (Windows CI #191; manual Editor test deferred)
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

**UI System is active.** UI Canvas/Panel/Text/Button authoring, serialization, Inspector properties, aspect-preserving layout, Editor viewport preview, Canvas-aware pointer routing and runtime Component click dispatch are implemented. Windows CI #226–#228 passed; click-dispatch hardening is committed in `79c6ab9` and `663d9df` with CI #229/#230 running. Final interactive Windows Editor verification remains in [UI_RUNTIME_VERIFICATION.md](UI_RUNTIME_VERIFICATION.md).

**Multiple Colliders per Entity is complete.** Model, serialization/migration, runtime/query filtering, Entity-pair contact aggregation, Inspector editing and multi-shape Debug Overlay passed their respective Windows CI runs (#135, #151, #155, #164, #169, #170). The user confirmed the Windows Editor checklist passed on 2026-11-27, including Editor launch/layout, Entity and Transform editing, duplicate/delete, PhysicsBody2D Inspector add/edit/remove, ColliderSet2D editing, Physics Debug Overlay, save/reopen persistence, and Play/Stop behavior.

**Audio implementation is complete pending final interactive verification.** The miniaudio 0.11.26 runtime facade, Application/Editor audio lifecycle, device-less generated-WAV tests, serializable `AudioSource2D`, typed Inspector properties and safe Audio-asset picker, Scene asset-resolution/playback lifecycle, runtime Play/Stop controls, safe Scene move/Stop lifetime, and Editor Asset Browser preview are implemented. Windows CI #219 passed. Automated coverage includes unavailable/uninitialized devices, missing dependencies/assets, inactive/disabled sources, invalid volume, wrong asset types and hand-edited traversal metadata. Run the final Windows Editor checklist in [AUDIO_RUNTIME_VERIFICATION.md](AUDIO_RUNTIME_VERIFICATION.md) before closing the milestone. Sprite Animation is complete (CI #199 Success and user-confirmed checks passed). Physics Joints automated tests passed CI #191; manual Editor verification remains deferred.

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
| 2026-11-27 | [UI Button click dispatch, `9e2c969`, CI #225](https://github.com/jonjud/ember/actions/runs/46011291135) | Success; runtime Component callback dispatch and initial Editor verification checklist prepared |\n| 2026-11-27 | [UI callback integration tests, `0320a08`, CI #226](https://github.com/jonjud/ember/actions/runs/46013967857) | Success |\n| 2026-11-27 | [Viewport-aware UI input, `600f407`, CI #227](https://github.com/jonjud/ember/actions/runs/46014213385) | Success |\n| 2026-11-27 | [UI status/roadmap refresh, CI #228](https://github.com/jonjud/ember/actions/runs/46014412490) | Success |\n| 2026-11-27 | [Guard UI click dispatch, `79c6ab9`, CI #229](https://github.com/jonjud/ember/actions/runs/46015014246) | In progress |\n| 2026-11-27 | [Test non-Button click rejection, `663d9df`, CI #230](https://github.com/jonjud/ember/actions/runs/46015090528) | In progress |

---

**Handoff:** UI System implementation is on `develop` through `663d9df`. Windows CI #226–#228 passed; runtime click dispatch now validates that the target is an enabled, interactable, valid UIButton2D before invoking callbacks. CI #229/#230 are verifying this hardening. The remaining UI gate is the one-time Windows Editor checklist in [UI_RUNTIME_VERIFICATION.md](UI_RUNTIME_VERIFICATION.md). Audio runtime implementation is on `develop` with Windows CI #219 green; its final interactive checklist remains in [AUDIO_RUNTIME_VERIFICATION.md](AUDIO_RUNTIME_VERIFICATION.md). Sprite Animation is complete (CI #199 Success and user-confirmed checks passed). Physics Joints automated tests passed CI #191; its manual Editor checklist remains deferred to the final test session. Work on `develop` only; do not merge to `main`.
