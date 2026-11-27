# Multiple Colliders per Entity — Design

## Goal

Allow one Entity with one PhysicsBody2D to own multiple Box and/or Circle collider shapes. Each shape has independent local geometry, material, sensor state, and query-filter bits. PhysicsWorld remains backend-neutral and Box2D stays private.

## Chosen configuration model

Introduce one serializable `ColliderSet2D` Component per Entity. It owns an ordered collection of collider entries. Each entry contains:

- Stable `ColliderId` (64-bit, nonzero, unique within the set).
- Shape kind: Box or Circle.
- Local offset; Box also has half-extents and local rotation, Circle has radius.
- Density, friction, restitution and sensor flag.
- Query category bits and mask bits.

A single set avoids requiring duplicate instances of the same C++ Component type, which the current Entity/Component registry does not support. The set is cloned and serialized as configuration only; no Box2D shape IDs or runtime handles are stored in it.

The existing `BoxCollider2D` and `CircleCollider2D` remain supported as legacy authoring/serialization inputs during migration. When loading a legacy scene, convert either legacy component into a one-entry ColliderSet2D while preserving its settings. If a scene contains both legacy collider types, preserve current behavior by reporting a diagnostic rather than silently choosing one. Saving after migration writes the canonical ColliderSet2D representation. Migration must be versioned and covered by fixtures.

## Runtime identity and mapping

- One Entity still owns exactly one PhysicsBody2D and one PhysicsBodyHandle.
- PhysicsWorld adds each collider entry as a separate shape on that body.
- Collider IDs are stable configuration IDs; backend shape IDs remain private and transient.
- Runtime mappings associate (PhysicsBodyHandle, ColliderId) with backend shape state only inside PhysicsWorld/Scene.
- Destroying a body destroys all its shapes; stopping or rebuilding a Scene clears all runtime mappings.
- Cloning and serialization copy only ColliderSet2D values and stable ColliderIds.

## Events and queries

- Existing Entity-level Begin/End contact events remain the public baseline. Multiple shape contacts between the same entity pair must not produce duplicate Begin events while any shape pair remains touching, nor an End until the final touching shape pair ends.
- Existing RaycastClosest and OverlapAABB remain entity-level and deduplicate results by EntityId, even when multiple shapes on one body are hit.
- A later extension may expose ColliderId in detailed contact/query results, but that is not required for the first multi-collider slice.
- Each shape receives its own category/mask query filter.

## Editor and serialization

- Inspector presents a collider list with Add Box, Add Circle, duplicate, and remove actions; each entry exposes its shape-specific geometry and shared material/filter properties.
- Physics debug overlay draws every configured entry, using existing solid/sensor colors.
- Scene serializer writes stable IDs and canonical set data. Legacy box/circle component data migrates to a one-entry set.
- Registry/Inspector changes must not require ImGui dependencies in ember_engine.

## Implementation sequence

1. [x] Add ColliderSet2D value model, stable ColliderId generation/validation, Clone behavior and unit tests (CI #135).
2. [x] Add canonical serialization and legacy Scene migration fixtures (CI #151).
3. [x] Extend PhysicsWorld shape creation to accept per-shape query filters and preserve backend privacy (CI #155).
4. [x] Build multiple shapes per body in Scene; create each ColliderSet2D entry as a shape on one body.
5. [x] Keep raycast/overlap mapping Entity-level and deduplicate multi-shape results.
6. [ ] Aggregate shape-pair contact transitions into one Entity-pair Begin/End (implementation and regression test committed; CI verification pending).
7. [ ] Add Inspector list editing and draw all shapes in the debug overlay.
8. [ ] Run Windows CI and the interactive multi-collider runtime checklist.

## Non-goals

- Joints, compound body hierarchies, runtime mutation while Scene is running, per-collider script callbacks, and exposing Box2D shape IDs.
