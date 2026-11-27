# EMBER Physics Runtime — Windows Verification Checklist

This checklist verifies the first interactive physics slice in the actual Windows Editor. A successful CI build is not a substitute for these checks.

## Preparation

1. Pull the latest `develop` branch.
2. Configure and build the `ember_editor` target using the repository's Windows CMake workflow.
3. Launch the Editor and open or create a test scene.
4. Keep a copy of the scene before testing Play Mode.

## A. Physics Debug Overlay

Create separate entities with the following configurations using the Inspector:

- **Box solid:** enabled `PhysicsBody2D` + `BoxCollider2D`, sensor off.
- **Box sensor:** enabled `PhysicsBody2D` + `BoxCollider2D`, sensor on.
- **Circle solid:** enabled `PhysicsBody2D` + `CircleCollider2D`, sensor off.
- **Circle sensor:** enabled `PhysicsBody2D` + `CircleCollider2D`, sensor on.

| ID | Action | Expected result |
|---|---|---|
| A1 | Turn on the `Physics` checkbox in the Viewport toolbar | Collider outlines appear; render geometry remains visible |
| A2 | Inspect solid box and circle | Outlines are orange |
| A3 | Inspect sensor box and circle | Outlines are cyan |
| A4 | Change box half-extents and circle radius | Outline dimensions update immediately |
| A5 | Change collider offset | Outline moves relative to the entity origin |
| A6 | Rotate the entity; then change the box's local rotation | Box outline follows combined entity + collider rotation |
| A7 | Change circle offset and rotate its entity | Circle center follows the rotated local offset; radius remains circular |
| A8 | Turn off the `Physics` checkbox | All physics outlines disappear without hiding normal entity rendering |
| A9 | Disable a PhysicsBody2D component | Its collider outline is not drawn |
| A10 | Configure both BoxCollider2D and CircleCollider2D on one entity | Invalid dual-collider configuration is not drawn |

## B. Play Mode and Scene Isolation

1. Create a Dynamic body with a collider above a Static body with a collider.
2. Save the authoring scene.
3. Press **F5** to start Play Mode.
4. Observe the Dynamic body fall and collide with the Static body.
5. While playing, verify the authoring Scene remains protected from Inspector edits and authoring-only operations.
6. Press **Shift+F5** to stop Play Mode.
7. Verify the authoring scene returns with its original transforms and component configuration.

| ID | Action | Expected result |
|---|---|---|
| B1 | Start Play Mode with F5 | Play Mode starts without crash; runtime Scene clone owns its physics world |
| B2 | Observe Dynamic body over Static collider | Dynamic body falls and is stopped by the collider |
| B3 | Stop with Shift+F5 | Play Mode stops cleanly |
| B4 | Inspect authoring Scene after stop | Original entity transforms and component settings are unchanged |
| B5 | Start and stop Play Mode repeatedly | No crash, stale physics state, or visible duplication |

## Reporting

Record each ID as **PASS**, **FAIL**, or **BLOCKED**. For failures, include:

- Windows version and build configuration (Debug/Release)
- Exact checklist ID
- Reproduction steps
- Screenshot or short recording when visual
- Relevant console/output message

Do not mark the interactive milestone complete until all applicable checks pass. CI status should be recorded separately from this manual result.
