# Physics Joints — Initial Design

## Goal

Add entity-to-entity constraints to EMBER's 2D physics runtime without exposing Box2D types or IDs through public engine APIs. The first vertical slice supports Revolute and Distance joints, including serialization, Scene lifecycle, automated tests and basic Inspector editing.

## Design principles

- Box2D remains a private implementation detail behind `PhysicsWorld`.
- Joint configuration is authored data; backend joint IDs/handles are runtime-only and never serialized or copied as live handles.
- A joint connects two distinct entities that each have an enabled `PhysicsBody2D` and a successfully created runtime body.
- The component lives on the owning Entity and stores the other EntityId. The owning entity is body A; the referenced entity is body B.
- Scene structure is immutable while running, matching the current body/collider lifecycle.
- Invalid or unresolved references produce a Scene diagnostic and skip only that joint; they must not crash or invalidate unrelated bodies.
- Joint creation happens only after all bodies and colliders have been created. Joint destruction happens before body/world teardown.
- Destroying/rebuilding a PhysicsWorld clears all runtime joint mappings.

## First joint types

### RevoluteJoint2D

A pin/hinge constraint allowing relative rotation around an anchor.

Authored properties:
- `connectedEntity`: EntityId of body B.
- `localAnchorA`, `localAnchorB`: local-space anchor positions in pixels.
- `enableLimit`, `lowerAngle`, `upperAngle`: radians.
- `enableMotor`, `motorSpeed`: radians per second.
- `maxMotorTorque`: nonnegative torque.
- `collideConnected`: whether the two connected bodies may collide.

### DistanceJoint2D

A fixed/rest-length constraint between two local anchors.

Authored properties:
- `connectedEntity`: EntityId of body B.
- `localAnchorA`, `localAnchorB`: local-space anchor positions in pixels.
- `length`: positive distance in pixels; if a future UX supports automatic length, it must be resolved at authoring time rather than storing an ambiguous runtime default.
- `collideConnected`: whether the two connected bodies may collide.

Spring frequency/damping, length ranges, and other advanced controls are deferred until the basic constraint behavior is stable.

## Public PhysicsWorld API boundary

Add engine-owned descriptors and opaque handles:
- `PhysicsJointHandle` with a nonzero engine-generated value.
- `PhysicsRevoluteJointDefinition2D` and `PhysicsDistanceJointDefinition2D`, each referencing two `PhysicsBodyHandle` values and carrying meter/radian values.
- `CreateRevoluteJoint`, `CreateDistanceJoint`, and `DestroyJoint` methods.
- Invalid handles, same-body constraints, non-finite anchors/angles/speeds, invalid limits, and nonpositive lengths are rejected safely.

The implementation maps opaque handles to Box2D joint IDs privately. Destroying a body must not leave stale public joint mappings; either reject body destruction while attached joints exist or deterministically destroy dependent joints first. Prefer deterministic dependent-joint destruction.

## Scene integration

- Build all bodies/colliders first.
- Iterate entities in stable EntityId order to create joint components.
- Resolve both entity references through the Scene's body map.
- Convert anchor positions and distance from pixels to meters; angles remain radians.
- Track runtime joint handles separately from authored components.
- Destroy joints before bodies during Stop/rebuild.
- Clear contact/joint runtime state on teardown.
- Keep Play Mode behavior isolated through the existing Scene clone model.

## Serialization and migration

- Register `RevoluteJoint2D` and `DistanceJoint2D` as canonical component types.
- Serialize only EntityId references and authored values.
- Validate enum/range/numeric values on load; preserve valid stable EntityIds.
- Missing target entities remain loadable but produce a runtime diagnostic when the Scene starts, matching the engine's tolerant runtime setup behavior.
- Add round-trip tests and invalid-reference fixtures.

## Inspector

First-pass Inspector controls:
- Connected EntityId field.
- Local anchors as Vector2.
- Revolute limits/motor toggles and numeric values.
- Distance length.
- Collide Connected toggle.

A searchable entity picker may be added later; the initial UI can use a validated numeric EntityId field if the existing Inspector property system supports it cleanly.

## Implementation sequence

1. [x] Complete multiple colliders per entity and confirm Windows interactive verification.
2. [ ] Add opaque PhysicsJointHandle and PhysicsWorld Revolute/Distance creation/destruction with validation and unit tests.
3. [ ] Add serializable RevoluteJoint2D and DistanceJoint2D components with clone/validation tests.
4. [ ] Register component serializers and Scene round-trip tests.
5. [ ] Integrate joint creation after body creation and teardown before body destruction; add Scene lifecycle tests.
6. [ ] Add Inspector editing and clear diagnostics for unresolved/invalid connections.
7. [ ] Run Windows CI and interactive Editor/Play Mode verification.

## Deferred

Prismatic, weld, wheel, motor, mouse joints; joint break events; runtime joint mutation; scripting callbacks; advanced spring tuning; joint visualization/debug gizmos.
