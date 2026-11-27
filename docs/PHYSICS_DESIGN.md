# EMBER Physics Integration Design

## Status
- Backend: Box2D **v3.2.0**, pinned by CMake FetchContent.
- Complete: private dependency boundary, PhysicsWorld RAII facade/body/collider API and automated tests.
- Complete: serializable PhysicsBody2D, BoxCollider2D and CircleCollider2D configuration components, registry/serialization and Inspector properties.
- Complete: Scene-owned PhysicsWorld, entity body construction, fixed-step simulation, transform synchronization and teardown.
- Complete: Scene-level tests for lifecycle, falling/collision, invalid collider combinations, non-finite frame delta, stop and clone boundaries.
- Complete: numeric body/collider configuration validation, disabled/static/kinematic behavior tests, and Editor collider debug overlay (Windows CI #113).
- Complete: engine-owned Begin/End contact event API in PhysicsWorld and Scene, with EntityId pairs; Windows CI run 45939999840 passed.
- Complete: engine-owned closest raycast and AABB overlap queries, including category/mask query filters and Scene EntityId mapping; Windows CI run 45941862568 passed.
- Complete: automated category/mask filter coverage, sensor inclusion in raycast/AABB overlap queries, and Scene filter-forwarding tests; PhysicsWorld query coverage passed Windows CI #126.
- Active milestone: multiple colliders per entity. ColliderSet2D model and stable IDs passed CI #135. Canonical ColliderSet2D serialization and migration of legacy single Box/Circle components during Scene loading are implemented; runtime multi-shape integration is pending CI.
- Active milestone: PhysicsJointHandle, Revolute/Distance PhysicsWorld API, serializable RevoluteJoint2D/DistanceJoint2D components, EntityId references, Scene runtime creation/teardown, and Inspector reference/settings controls are implemented; Windows CI #191 passed implementation and automated tests; final Windows manual verification remains deferred until the planned test session.

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
- Scene creates all eligible bodies first, then resolves joint component EntityId references and creates runtime joints. Missing, self, disabled, or otherwise invalid body references are skipped with diagnostics.
- Joint handles are runtime-only and owned by PhysicsWorld; Scene Stop destroys the PhysicsWorld, which tears down joints and bodies together.
- PhysicsWorld is created on Scene Start and destroyed on Scene Stop.
- Body handles and backend IDs are runtime-only; never serialize or clone them.
- Scene builds bodies/colliders from component configuration at Start, steps the world during Update, then synchronizes simulated transforms.
- Scene structural changes remain prohibited while running.
- Scene-based test executables must link PhysicsWorld implementation and Box2D, not only Scene.cpp.

## Engine-facing components and API
- `PhysicsBody2D`: body type (static, kinematic, dynamic), gravity scale, linear/angular damping and enabled flag.
- `BoxCollider2D`: half extents, local offset/rotation, density, friction, restitution and sensor flag.
- `CircleCollider2D`: radius, local offset, density, friction, restitution and sensor flag.
- `ColliderSet2D`: canonical ordered Box/Circle entries with stable IDs, independent material/sensor/query-filter values; serialized without runtime backend handles. Legacy BoxCollider2D/CircleCollider2D Scene data migrates to one-entry sets on load.
- `RevoluteJoint2D`: connected EntityId, local anchors, optional angle limits, optional motor speed/torque and collide-connected flag.
- `DistanceJoint2D`: connected EntityId, local anchors, authored length in pixels and collide-connected flag.
- `PhysicsWorld`: gravity, body/joint create/destroy, box/circle collider creation, transform read/write, stepping, consuming Begin/End contact events, closest raycasts, AABB overlap queries and body/joint counts.
- Components store configuration only; runtime handles are owned by Scene.
- Scene loading migrates one legacy BoxCollider2D or CircleCollider2D into a canonical ColliderSet2D while preserving geometry/material settings. Entities with both legacy shapes, or a legacy shape plus a set, fail loading with a diagnostic rather than silently dropping data. See [MULTIPLE_COLLIDERS_DESIGN.md](MULTIPLE_COLLIDERS_DESIGN.md).

## Implemented runtime behavior
- Create a body for an enabled PhysicsBody2D and attach its configured collider.
- A collider without PhysicsBody2D is skipped with a diagnostic.
- Until the multi-collider migration lands, entities with both legacy collider components are skipped with a diagnostic.
- Failed body/collider creation is caught per entity; partial body state is destroyed and a diagnostic is recorded.
- Fixed timestep: 1/60 second. Render delta is clamped to 0.25 seconds; at most 8 catch-up substeps are processed per frame.
- Dynamic bodies are physics-authoritative: after stepping, write body position/rotation to Entity Transform2D (pixels/radians).
- Static and kinematic bodies are transform-authoritative: update their physics transforms from Entity Transform2D before stepping.
- After each fixed simulation step, Scene consumes contact events and maps PhysicsBodyHandles to EntityId pairs. GetPhysicsContactEvents() provides the events from the latest Update and is cleared at the next Update.
- When a transform-driven static/kinematic body moves, PhysicsWorld wakes dynamic bodies with active contacts against it so contact separation is processed and an End event can be emitted.
- Scene Stop clears body handles and contact state, destroys PhysicsWorld (including all runtime joints), and resets the accumulator.
- Joint local anchors and DistanceJoint2D length are authored in pixels and converted to meters at the Scene boundary. Revolute angles and motor speed remain radians-based.
- Non-finite frame delta is ignored safely.

## Remaining validation work
- Existing Editor/Play Mode and multi-collider workflows passed the user-run Windows checklist; automated Windows CI #132 and subsequent physics runs passed.
- Physics Joints automated build/test confirmation passed Windows CI #191. Defer interactive Windows Editor verification for joints until the planned final test session.

## Milestone order
1. Pin Box2D and establish private dependency boundary. (Complete)
2. Implement PhysicsWorld facade and tests. (Complete)
3. Add serializable physics components, registry, Inspector controls. (Complete)
4. Integrate Scene Start/Update/Stop, fixed timestep and transform synchronization. (Complete; CI #109)
5. Harden numeric validation and add body-mode edge-case tests. (Complete)
6. Add Editor collider debug drawing. (Complete; CI #113)
7. Add collision/contact begin/end event API. (Complete; Windows CI run 45939999840)
8. Add raycast and overlap query APIs. (Complete; PhysicsWorld and Scene query tests; Windows CI #121)
9. Expand query-filter and sensor-specific automated test coverage. (Complete; Windows CI #126)
10. Verify Editor/Play Mode physics behavior interactively on Windows. (Complete; user-confirmed checklist pass, CI #132)
11. Implement multiple colliders per entity. (Active)
12. Consider joints after the multi-collider slice is stable.

## Explicit non-goals for first runtime slice
- 3D physics, joints, ragdolls, character controllers, networking/rollback and multithreaded jobs.
- Exposing Box2D handles to gameplay code.
- Automatically converting RenderComponent2D into collision geometry.
- Joints.
