# EMBER Physics Integration Design

## Status
- Backend: Box2D **v3.2.0**, pinned by CMake FetchContent.
- Complete: private dependency boundary, PhysicsWorld RAII facade/body/collider API and automated tests.
- Complete: serializable PhysicsBody2D, BoxCollider2D and CircleCollider2D configuration components, registry/serialization and Inspector properties.
- Next: Scene-owned PhysicsWorld, body construction, fixed-step simulation and transform synchronization.
- Not implemented: Scene runtime simulation, collision event API, Editor debug drawing, joints and multiple colliders per entity.

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
- Build physics bodies from component configuration at Start, step the world, then synchronize simulated transforms back to entities.
- Scene structural changes remain prohibited while running.

## Engine-facing components and API
- `PhysicsBody2D`: body type (static, kinematic, dynamic), gravity scale, linear/angular damping and enabled flag.
- `BoxCollider2D`: half extents, local offset/rotation, density, friction, restitution and sensor flag.
- `CircleCollider2D`: radius, local offset, density, friction, restitution and sensor flag.
- `PhysicsWorld`: gravity, body create/destroy, box/circle collider creation, transform read/write, stepping and body count.
- Components store configuration only; runtime handles are owned by Scene.
- Initial scope is one collider per entity. An entity with both BoxCollider2D and CircleCollider2D is invalid for the first slice and must produce a clear diagnostic.

## Runtime integration rules
- Create a body only when the entity has an enabled PhysicsBody2D.
- A collider without PhysicsBody2D is invalid and should be diagnosed, not silently auto-promoted.
- A disabled PhysicsBody2D creates no body.
- Validate all numeric configuration before creating runtime objects. Reject non-finite values, negative damping/material parameters and non-positive shape dimensions/radius.
- Fixed timestep target: 1/60 second. Accumulate render delta with a cap and bound the number of catch-up steps.
- Dynamic bodies are physics-authoritative: after stepping, write body position/rotation to Entity Transform2D (pixels/radians).
- Static and kinematic bodies are transform-authoritative initially: update their physics transforms from Entity Transform2D before stepping. This avoids bidirectional feedback loops.
- If a body or collider fails to create, tear down any partial runtime state and report a diagnostic; never leave an entity with half-created physics state.

## Milestone order
1. Pin Box2D and establish private dependency boundary. (Complete)
2. Implement PhysicsWorld facade and tests. (Complete)
3. Add serializable physics components, registry, Inspector controls. (Complete)
4. Integrate Scene Start/Update/Stop, fixed timestep and transform synchronization. (Next)
5. Add Scene-level tests for lifecycle, invalid configuration, simulation and serialization boundaries.
6. Add editor debug drawing/contact/query APIs after core runtime is stable.

## Explicit non-goals for first runtime slice
- 3D physics, joints, ragdolls, character controllers, networking/rollback and multithreaded jobs.
- Exposing Box2D handles to gameplay code.
- Automatically converting RenderComponent2D into collision geometry.
- Multiple colliders per entity.
