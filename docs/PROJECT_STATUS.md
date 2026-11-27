# EMBER Engine — Living Project Status

> เอกสารนี้เป็น Single Source of Truth สำหรับสถานะงานและการส่งต่องานระหว่างบทสนทนา  
> ดูภาพรวมที่ [ROADMAP.md](ROADMAP.md), โครงสร้างที่ [ARCHITECTURE.md](ARCHITECTURE.md), และ Physics decisions ที่ [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md)

## 1. สถานะปัจจุบัน

| รายการ | ค่า |
|---|---|
| Repository | [jonjud/ember](https://github.com/jonjud/ember) |
| Active branch | `develop` |
| Stable branch | `main` |
| Last code commit | [`9c8160e`](https://github.com/jonjud/ember/commit/9c8160e86e0e3a91284a6a24bcff4db5ca593e1b) — Vector2/Enumeration Inspector properties |
| Last verified CI | [Windows Build #103](https://github.com/jonjud/ember/actions/runs/45933429501) — **Success** (Configure, Build, CTest) |
| Last completed milestone | Physics Component Configuration + Registry/Inspector integration |
| Current active milestone | Physics Runtime Integration — Scene lifecycle and fixed timestep |
| Physics backend | Box2D v3.2.0, pinned via FetchContent; wrapped by EMBER-owned PhysicsWorld API |

> CI Success ยืนยันเฉพาะ Windows build และ automated tests ไม่ได้ยืนยันการทดสอบ Editor/Runtime แบบ interactive บนเครื่องจริง

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
- [ ] Create physics bodies from Scene entities and synchronize transforms
- [ ] Add fixed-step simulation to running Scene
- [ ] Add Scene-level physics integration tests and debug drawing

**Important boundary:** Physics components currently store configuration only. They do not own Box2D IDs or runtime body handles. Scene does not yet create or step a PhysicsWorld.

## 3. Physics decisions to preserve

- Box2D v3.2.0 is private; Box2D types must not leak into public Scene/Entity/Component/Editor APIs.
- A running Scene owns its PhysicsWorld; Editor Play Mode operates on an isolated Scene clone.
- Physics units are meters; conversion is 100 pixels = 1 meter.
- EMBER world uses positive Y downward; default gravity points positive Y.
- Entity Transform2D position maps to body origin; rotation is radians.
- Collider geometry is explicit, never inferred from sprite/render size.
- Physics handles are runtime-only and never serialized or cloned as raw backend IDs.
- Target fixed timestep is 1/60 s with bounded accumulator and substeps.
- Initial slice supports one collider per entity; if both Box and Circle colliders are present, runtime should reject the ambiguous configuration clearly rather than silently choose one.

## 4. Next milestone — Physics Runtime Integration

1. Add Scene-owned runtime PhysicsWorld and entity-to-body handle mapping.
2. Build bodies/colliders on Scene Start from PhysicsBody2D and collider configuration.
3. Define behavior for invalid configurations, missing PhysicsBody2D, disabled bodies and duplicate collider types; surface safe diagnostics.
4. Advance physics using a fixed 1/60 s timestep with capped frame delta and bounded substeps.
5. Sync dynamic body transforms back to Entity Transform2D; define static/kinematic authority to prevent feedback loops.
6. Destroy runtime physics state on Scene Stop before component configuration is released.
7. Add tests for creation, invalid config, fixed stepping, transform sync, clone/serialization boundaries and teardown.

Do not begin Editor debug drawing, joints, multiple colliders, or 3D physics until this vertical slice is stable.

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
| 2026-11-27 | [Physics registration/Inspector, CI #103](https://github.com/jonjud/ember/actions/runs/45933429501) | Success; Vector2 and Enumeration controls included |

---

**Handoff:** Start by checking the latest `develop` commit and CI, then implement only the Scene runtime integration described above.
