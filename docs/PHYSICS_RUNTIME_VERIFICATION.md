# EMBER Physics Runtime — Windows Verification Checklist

This checklist verifies the Windows Editor workflow and the physics Inspector/runtime slices in the actual Windows Editor. A successful CI build is not a substitute for these checks.

## Recorded manual result — 2026-11-27

**User-reported result: PASS for the checklist run in this development session.** Confirmed areas: Editor launch and main layout; Scene Hierarchy, Inspector and Viewport; Entity creation/selection, Transform editing, duplicate/delete; PhysicsBody2D add/edit/remove; ColliderSet2D add/edit/duplicate/remove; Physics Debug Overlay; scene save/reopen; and Play/Stop behavior. This is a user-reported interactive Windows result, recorded separately from CI. It does not imply that the newly committed Physics Joints API has been tested in the Editor.

## Preparation

1. Pull the latest `develop` branch.
2. Configure and build the `ember_editor` target using the repository's Windows CMake workflow.
3. Launch the Editor and open or create a test scene.
4. Keep a copy of the scene before testing Play Mode.

## B. Physics Debug Overlay

Create separate entities with `PhysicsBody2D` + `ColliderSet2D`. Include one set containing at least two Boxes and one Circle, with a mix of solid and sensor entries. Also keep a legacy single-collider scene/entity available to verify fallback compatibility.

| ID | Action | Expected result |
|---|---|---|
| C1 | Turn on the `Physics` checkbox in the Viewport toolbar | Collider outlines appear; render geometry remains visible |
| C2 | Inspect solid box and circle | Outlines are orange |
| C3 | Inspect sensor box and circle | Outlines are cyan |
| C4 | Change box half-extents and circle radius | Outline dimensions update immediately |
| C5 | Change collider offset | Outline moves relative to the entity origin |
| C6 | Rotate the entity; then change the box's local rotation | Box outline follows combined entity + collider rotation |
| C7 | Change circle offset and rotate its entity | Circle center follows the rotated local offset; radius remains circular |
| C8 | Turn off the `Physics` checkbox | All physics outlines disappear without hiding normal entity rendering |
| C9 | Disable a PhysicsBody2D component | Its collider outline is not drawn |
| C10 | Create a set containing multiple Boxes and Circles | Every configured shape is drawn; no shape is omitted or duplicated |
| C11 | Set one entry as Sensor and leave another solid | Sensor outline is cyan; solid outline is orange |
| C12 | Edit one entry's offset, half-extents/radius, and Box rotation | Only the selected shape's outline updates and matches its Inspector values |
| C13 | Add Box and Circle entries, duplicate an entry, then remove one | List and overlay update immediately; duplicate receives a new stable ID |
| C14 | Save, close/reopen, and inspect the scene | Collider order, IDs, geometry, materials, sensor and filter settings persist |
| C15 | Load a legacy scene with one BoxCollider2D or CircleCollider2D | It migrates to a one-entry ColliderSet2D and remains visible |
| C16 | Use an entity with a ColliderSet2D and legacy collider data together | Conflicting data is rejected/reported according to migration rules; no silent duplicate rendering |

## C. Play Mode and Scene Isolation

1. Create a Dynamic body with a ColliderSet2D containing at least two shapes above a Static body with a multi-shape ColliderSet2D.
2. Save the authoring scene.
3. Press **F5** to start Play Mode.
4. Observe the Dynamic body fall and collide with the Static body.
5. While playing, verify the authoring Scene remains protected from Inspector edits and authoring-only operations.
6. Press **Shift+F5** to stop Play Mode.
7. Verify the authoring scene returns with its original transforms and component configuration.

| ID | Action | Expected result |
|---|---|---|
| C1 | Start Play Mode with F5 | Play Mode starts without crash; runtime Scene clone owns its physics world |
| C2 | Observe Dynamic body over Static collider | Dynamic body falls and is stopped by the collider |
| C3 | Stop with Shift+F5 | Play Mode stops cleanly |
| C4 | Inspect authoring Scene after stop | Original entity transforms and component settings are unchanged |
| C5 | Start and stop Play Mode repeatedly | No crash, stale physics state, or visible duplication |
| C6 | Run two entities with overlapping multiple shape pairs | Public contact events remain one Entity-pair Begin while any pair touches, then one End after the final pair separates |
| C7 | Stop Play Mode after physics moves dynamic entities | Authoring transforms and ColliderSet2D settings remain unchanged |

## Reporting

Record each ID as **PASS**, **FAIL**, or **BLOCKED**. The dated result above records the user's checklist run; keep future runs separately dated. For failures, include:

- Windows version and build configuration (Debug/Release)
- Exact checklist ID
- Reproduction steps
- Screenshot or short recording when visual
- Relevant console/output message

Do not mark the interactive milestone complete until all applicable checks pass. CI status should be recorded separately from this manual result.
