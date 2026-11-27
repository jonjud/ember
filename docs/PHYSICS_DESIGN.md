# EMBER Physics Integration Design

## Status

- Backend selected: Box2D **v3.2.0**, pinned by CMake FetchContent.
- Completed stage: dependency and architecture boundary.
- Current stage: PhysicsWorld wrapper and automated tests.
- Not implemented yet: PhysicsWorld wrapper, bodies, colliders, Scene synchronization, serialization, Inspector controls, and debug drawing.
- This document records design decisions before simulation code is added.

## Why Box2D 3.2.0

- It is the current upstream stable release selected for this integration.
- Version 3 uses a C API and opaque IDs instead of the older C++ pointer-oriented API.
- EMBER is written in C++20, but a small adapter can wrap the C API and expose C++-style engine-owned types.
- Pinning the tag avoids silently changing physics behavior when upstream releases a new version.

Upstream: https://github.com/erincatto/box2d/releases/tag/v3.2.0

## Dependency boundary

- Box2D headers and IDs must not appear in public EMBER Scene, Entity, Component, or Editor headers.
- `ember_engine` owns the adapter and links Box2D privately.
- Editor code talks only to engine-owned debug/query interfaces; it must not call Box2D directly.
- Box2D's own samples, docs, benchmarks, and unit-test targets are disabled in the FetchContent build. EMBER owns integration tests.

## Coordinate and unit convention

- Existing EMBER transforms and render dimensions are expressed in editor/world pixels.
- Physics uses meters. Initial conversion: **100 pixels = 1 meter**.
- Physics API values are meters, meters/second, radians, and seconds. Conversion occurs only at the adapter/Scene boundary.
- EMBER's 2D world uses positive Y downward to match the existing screen-space convention; default gravity is therefore positive Y.
- Collider dimensions are explicit component data, not implicitly inferred from RenderComponent2D size. This keeps invisible collision geometry possible and avoids surprising physics changes when art size changes.
- Body origin maps to Entity Transform2D position; rotation maps in radians.

## Ownership and lifecycle

- A running Scene owns one PhysicsWorld. The authoring Scene remains isolated from Play Mode because the Editor already plays an independent Scene clone.
- PhysicsWorld is created on Scene Start and destroyed on Scene Stop.
- Body and shape handles are runtime-only. They are never serialized or copied as raw Box2D IDs.
- Scene starts by creating bodies from physics configuration components; each fixed physics step advances the world; transforms are synchronized back to active entities after stepping.
- Scene stop destroys runtime physics state before component configuration is released.
- Structural Scene changes remain prohibited while running, matching the current Entity/Component lifecycle rules.

## Planned engine-facing data

- PhysicsBody2D: body type (static, kinematic, dynamic), gravity scale, damping, fixed-rotation option, and runtime enable state.
- BoxCollider2D: half extents, local offset/rotation, density, friction, restitution, and sensor flag.
- CircleCollider2D: radius, local offset, density, friction, restitution, and sensor flag.
- PhysicsWorld: gravity, fixed timestep accumulator, substep count, create/destroy body, step, and read-back transforms.
- Initial scope is one collider per entity for the first vertical slice; multiple colliders and joints can follow without exposing Box2D types.

## Fixed timestep

- Physics simulation uses a fixed step (initial target 1/60 s), independent of render frame rate.
- Render delta is accumulated; the accumulator is capped to avoid an unbounded catch-up spiral after pauses or debugger breaks.
- The number of simulation steps per rendered frame is bounded.
- The first implementation should expose deterministic configuration and tests rather than add interpolation prematurely.

## Milestone order

1. Pin Box2D and establish this boundary. (Complete)
2. Implement RAII PhysicsWorld and engine-owned body/shape descriptors. (Current stage)
3. Add tests for world creation, body lifecycle, gravity, and box/circle collision.
4. Integrate Scene Start/Update/Stop and fixed timestep.
5. Add serializable PhysicsBody2D and collider configuration components plus Inspector descriptors.
6. Add editor debug drawing and contact/query events after the core path is stable.

## Explicit non-goals for the first slice

- 3D physics, joints, ragdolls, character controllers, networking/rollback, and multithreaded job-system integration.
- Exposing Box2D handles to gameplay code.
- Automatically converting every RenderComponent2D into a collider.
