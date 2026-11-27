# EMBER Physics Integration Design

## Status
- Backend: Box2D **v3.2.0**, pinned by CMake FetchContent.
- Complete: private dependency boundary, PhysicsWorld RAII facade/body/collider API and automated tests.
- Complete: serializable PhysicsBody2D, BoxCollider2D and CircleCollider2D configuration components, registry/serialization and Inspector properties.
- Complete: Scene-owned PhysicsWorld, entity body construction, fixed-step simulation, transform synchronization and teardown.
- Complete: Scene-level tests for lifecycle, falling/collision, invalid collider combinations, non-finite frame delta, stop and clone boundaries.
- Complete: numeric body/collider configuration validation, disabled/static/kinematic behavior tests, and Editor collider debug overlay (Windows CI #113).
- Next: collision/contact begin/end event API returning engine-owned EntityId pairs.
- Not implemented: raycasts/overlap queries, joints and multiple colliders per entity.

## Dependency boundary
- Box2D headers and IDs must not appear in public EMBER Scene, Entity, Component or Editor headers.
- `ember_engine` owns the adapter and links Box2D privately.
- Editor talks only to engine-owned interfaces; it never calls Box2D directly.
- Box2D samples/docs/benchmarks/tests are disabled in FetchContent; EMBER owns integration tests.

## Coordinate and unit convention
- Entity transforms and render dimensions use world pixels.
- Physics uses meters; **100 pixels = 1 meter**.
- Physics API values are meters, meters/second, radians and seconds. Convert only at the Scene/adapter boundary.
- Positive Y points downward; default gravity is positive Y.
- Collider dimensions are explicit configuration, not inferred from render size.
- Entity Transform2D position maps to body origin; rotation maps in radians.

## Ownership and lifecycle
- A running Scene owns one PhysicsWorld. Editor Play Mode uses an isolated Scene clone.
- PhysicsWorld is created on Scene Start and destroyed on Scene Stop.
- Body handles and backend IDs are runtime-only; never serialize or clone them.
- Scene builds bodies/colliders from component configuration at Start, steps the world during Update, then synchronizes simulated transforms.
- Scene structural changes remain prohibited while running.
- Scene-based test executables must link PhysicsWorld implementation and Box2D, not only Scene.cpp.

## Engine-facing components and API
- `PhysicsBody2D`: body type (static, kinematic, dynamic), gravity scale, linear/angular damping and enabled flag.
- `BoxCollider2D`: half extents, local offset/rotation, density, friction, restitution and sensor flag.
- `CircleCollider2D`: radius, local offset, density, friction, restitution and sensor flag.
- `PhysicsWorld`: gravity, body create/destroy, box/circle collider creation, transform read/write, stepping and body count.
- Components store configuration only; runtime handles are owned by Scene.
- Initial scope is one collider per entity. An entity with both BoxCollider2D and CircleCollider2D is invalid and receives a diagnostic.

## Implemented runtime behavior
- Create a body for an enabled PhysicsBody2D and attach its configured collider.
- A collider without PhysicsBody2D is skipped with a diagnostic.
- Both collider types on one entity are skipped with a diagnostic.
- Failed body/collider creation is caught per entity; partial body state is destroyed and a diagnostic is recorded.
- Fixed timestep: 1/60 second. Render delta is clamped to 0.25 seconds; at most 8 catch-up substeps are processed per frame.
- Dynamic bodies are physics-authoritative: after stepping, write body position/rotation to Entity Transform2D (pixels/radians).
- Static and kinematic bodies are transform-authoritative: update their physics transforms from Entity Transform2D before stepping.
- Scene Stop clears body handles, destroys PhysicsWorld and resets the accumulator.
- Non-finite frame delta is ignored safely.

## Remaining validation work
- Validate all numeric configuration before creating runtime objects. Reject non-finite values, negative damping/material parameters and non-positive shape dimensions/radius.
- Add tests for disabled bodies and static/kinematic transform authority.
- Consider stronger diagnostics for unsupported or invalid body/collider combinations.
- Keep failed creation atomic: no entity may retain partially created runtime physics state.

## Milestone order
1. Pin Box2D and establish private dependency boundary. (Complete)
2. Implement PhysicsWorld facade and tests. (Complete)
3. Add serializable physics components, registry, Inspector controls. (Complete)
4. Integrate Scene Start/Update/Stop, fixed timestep and transform synchronization. (Complete; CI #109)
5. Harden numeric validation and add body-mode edge-case tests. (Complete)
6. Add Editor collider debug drawing. (Complete; CI #113)
7. Add collision/contact begin/end event API. (Next)
8. Add raycast and overlap query APIs.
9. Consider multiple colliders and joints after the first 2D runtime slice is stable.

## Explicit non-goals for first runtime slice
- 3D physics, joints, ragdolls, character controllers, networking/rollback and multithreaded jobs.
- Exposing Box2D handles to gameplay code.
- Automatically converting RenderComponent2D into collision geometry.
- Multiple colliders per entity.
