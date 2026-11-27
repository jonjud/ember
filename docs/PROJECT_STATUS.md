# EMBER Engine — Living Project Status

> เอกสารนี้เป็น Single Source of Truth สำหรับสถานะงานและการส่งต่องานระหว่างบทสนทนา  
> ดูภาพรวมที่ [ROADMAP.md](ROADMAP.md), โครงสร้างที่ [ARCHITECTURE.md](ARCHITECTURE.md), และ Physics decisions ที่ [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md)

## 1. สถานะปัจจุบัน

| รายการ | ค่า |
|---|---|
| Repository | [jonjud/ember](https://github.com/jonjud/ember) |
| Active branch | `develop` |
| Stable branch | `main` |
| Last code commit | [`a566315`](https://github.com/jonjud/ember/commit/a566315603132a7db48454935b6c1393e05ff476) — Fix contact metadata type after query refactor |
| Last verified CI | [Windows CI run 45941862568](https://github.com/jonjud/ember/actions/runs/45941862568) — **Success** (Configure, Build, Run engine tests) |
| Last completed milestone | Raycast and Overlap Query API |
| Physics backend | Box2D v3.2.0, pinned via FetchContent; wrapped by EMBER-owned PhysicsWorld API |

> CI Success ยืนยัน Windows build และ automated tests ไม่ได้ยืนยันการทดสอบ Editor/Runtime แบบ interactive บนเครื่องจริง

## 2. สิ่งที่เสร็จแล้ว

### Engine / Editor foundations
- [x] CMake/C++20, SDL3 application loop, timing and input foundation
- [x] Renderer2D, Transform2D, Camera2D, Scene/Entity
- [x] AssetDirectory, AssetManager, AssetRegistry, texture cache and import metadata
- [x] Versioned project/scene/asset serialization and migration
- [x] Editor hierarchy, Inspector, viewport, gizmos, Play/Stop with isolated Scene clone
- [x] Generic Component lifecycle, clone, registry, factory, serialization and schema migrations
- [x] Add/remove registered Components through Editor UI
- [x] Typed Inspector property descriptors and widgets: bool, int, float, string, Vector2, Enumeration

### Physics — completed slices
- [x] Pin Box2D v3.2.0 and isolate it behind private engine implementation
- [x] Define units, coordinate conversion, ownership and lifecycle rules in [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md)
- [x] Implement engine-owned PhysicsWorld RAII facade and body/collider API
- [x] Automated PhysicsWorld tests
- [x] Add serializable configuration components: PhysicsBody2D, BoxCollider2D, CircleCollider2D
- [x] Register physics component types, serializers/deserializers and Inspector properties
- [x] Add Vector2 Inspector controls and enum dropdown support
- [x] Scene owns PhysicsWorld during runtime and builds bodies/colliders from entity components
- [x] Fixed 1/60 s simulation step with bounded accumulator, frame-delta clamp and substep cap
- [x] Dynamic body -> Entity Transform2D synchronization; static/kinematic transform -> physics synchronization
- [x] Scene Stop tears down runtime physics; runtime handles remain transient and are not cloned/serialized
- [x] Scene physics tests: lifecycle, falling/collision, invalid collider configurations, NaN delta, stop/clone behavior
- [x] Link PhysicsWorld and Box2D into all Scene-based test targets (CI linker fix)
- [x] Validate physics body/collider numeric configuration and keep failed collider creation atomic
- [x] Test disabled bodies and static/kinematic transform authority
- [x] Add Editor viewport Physics debug overlay for box/circle collider outlines (CI #113)
- [x] Add engine-owned PhysicsWorld Begin/End contact events mapped to Scene EntityIds
- [x] Add closest raycast and AABB overlap queries with engine-owned query filters/results and Scene EntityId mapping
- [x] Collect contact events after each fixed simulation step; Scene exposes the latest Update's events until next Update
- [x] Wake touching dynamic bodies when a transform-driven static/kinematic body moves so contact separation can be processed

## 3. Physics decisions to preserve
- Box2D v3.2.0 is private; Box2D types must not leak into public Scene/Entity/Component/Editor APIs.
- A running Scene owns its PhysicsWorld; Editor Play Mode operates on an isolated Scene clone.
- Physics units are meters; conversion is 100 pixels = 1 meter.
- EMBER world uses positive Y downward; default gravity points positive Y.
- Entity Transform2D position maps to body origin; rotation is radians.
- Collider geometry is explicit, never inferred from sprite/render size.
- Physics handles are runtime-only and never serialized or cloned as raw backend IDs.
- Fixed timestep is 1/60 s with bounded accumulator and substeps.
- Initial slice supports one collider per entity. Both Box and Circle colliders on one entity are rejected with a diagnostic.
- Collider without PhysicsBody2D is skipped with a diagnostic.
- Dynamic bodies are physics-authoritative; static and kinematic bodies are transform-authoritative.

## 4. Current limitations / next milestone

The Scene-owned 2D physics slice now includes the Editor collider debug overlay, engine-owned Begin/End contact events, and closest raycast/AABB overlap queries. These APIs expose only EMBER-owned handles/results; Scene maps physics bodies to EntityIds and converts pixel-space query inputs to meters. Query filters support category/mask bits, and automated tests cover nearest-hit selection, hit normal/fraction, invalid zero-length rays, overlap results and invalid bounds. The Windows CI run for this milestone (#121) passed Configure, Build and engine tests.

Next focused task: strengthen query-filter and sensor-specific test coverage, then consider a small Editor/runtime interaction verification pass. Multiple colliders and joints remain deferred.

Interactive Editor/Play Mode verification on a Windows machine is still outstanding; CI success does not replace that manual check.

## 5. Development rules
1. Work on `develop` only. Never merge to `main` without explicit approval.
2. Inspect latest `develop` HEAD before every implementation change.
3. Keep milestones small, buildable and testable.
4. After a milestone, update this file and [ROADMAP.md](ROADMAP.md); update [ARCHITECTURE.md](ARCHITECTURE.md) when behavior/ownership changes.
5. Never report CI passed until the workflow is completed with `conclusion: success`.
6. Distinguish CI build/tests from interactive runtime verification.
7. If CI fails, inspect logs, fix the root cause, and wait for a successful rerun before closing the milestone.

## 6. Recent milestone log

| Date | Commit / CI | Result |
|---|---|---|
| 2026-11-27 | [PhysicsWorld wrapper, CI #100](https://github.com/jonjud/ember/actions/runs/45925546626) | Success |
| 2026-11-27 | [Physics component config types, CI #101](https://github.com/jonjud/ember/actions/runs/45929634947) | Success |
| 2026-11-27 | [Physics registration/Inspector, CI #103](https://github.com/jonjud/ember/actions/runs/45933429501) | Success |
| 2026-11-27 | [Scene Physics Runtime Integration, CI #109](https://github.com/jonjud/ember/actions/runs/45935434409) | Success; Configure, Build and engine tests all passed |
| 2026-11-27 | [Physics Debug Overlay, CI #113](https://github.com/jonjud/ember/actions/runs/45937028116) | Success; Windows build and automated tests passed |
| 2026-11-27 | [Scene Contact Event API, CI run 45939999840](https://github.com/jonjud/ember/actions/runs/45939999840) | Success; Begin/End contact events and EntityId mapping verified by automated tests |
| 2026-11-27 | [Raycast and Overlap Query API, CI run 45941862568](https://github.com/jonjud/ember/actions/runs/45941862568) | Success; Configure, Build and engine tests passed |

---

**Handoff:** Start by checking latest `develop` HEAD and CI. Next focused task: strengthen Raycast/Overlap query-filter and sensor-specific test coverage; keep Box2D types private.
