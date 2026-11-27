# EMBER Engine Architecture

EMBER is a 2D-first engine written in C++20. It grows through small vertical slices and keeps editor-only code separate from the engine library.

## Current targets
- `ember_engine`: runtime-facing engine library; no Dear ImGui dependency.
- `ember_editor` (output executable `ember`): SDL3 + Dear ImGui editor.
- Focused test executables run through CTest in Windows CI. Scene-based test targets explicitly compile/link the PhysicsWorld implementation and Box2D backend.

## Source layout
```text
src/
├── Editor/
│   └── EditorApplication.hpp/.cpp
├── Engine/
│   ├── Core/       Application, Time
│   ├── Math/       Vector2
│   ├── Physics/    PhysicsWorld, PhysicsComponents2D, registration
│   ├── Platform/   Input, InputMap
│   ├── Project/    ProjectConfigurationStore
│   ├── Renderer/   Renderer2D, Camera2D
│   ├── Resources/  AssetDirectory, AssetManager, AssetRegistry,
│   │               AssetMetadataStore, AssetImportService, TextureCache
│   └── Scene/      Component, ComponentRegistry, Entity, Scene,
│                   Transform2D, RenderComponent2D, SceneSerializer
└── main.cpp

tests/
├── PhysicsWorldTests.cpp
├── PhysicsComponentsTests.cpp
├── ScenePhysicsTests.cpp
└── (other focused engine tests)
```

## Responsibilities and ownership
- **Core / Application:** runtime window, active Scene, AssetManager, and application event/update/render lifecycle.
- **Editor / EditorApplication:** editor panels, viewport, gizmos, scene workflow and Play Mode; editor UI does not own physics backend objects. The viewport Physics toggle draws configured box/circle collider outlines using engine component data only.
- **Math / Vector2:** engine-owned 2D vector type.
- **Scene / Component:** polymorphic runtime behavior with Clone, OnCreate, OnUpdate and OnDestroy hooks. Components are owned by entities; structural changes are locked while a Scene is running.
- **Scene / ComponentRegistry:** stable IDs, factories, versioned JSON serializers, schema migrations and UI-agnostic typed property descriptors. Current Inspector types include bool, int, float, string, Vector2 and Enumeration; the Editor maps these descriptors to ImGui controls.
- **Scene / Entity:** stable 64-bit ID, name, active state, Transform2D, render/movement data and registered runtime components.
- **Scene / Scene:** owns entities; supports create, duplicate, destroy, lookup, iteration, cloning and Start / Update / Stop lifecycle dispatch. While running, it owns one PhysicsWorld, an EntityId-to-PhysicsBodyHandle map, an accumulator and diagnostic messages.
- **Physics / PhysicsWorld:** EMBER-owned RAII facade around Box2D v3.2.0. Public descriptors and handles are engine-owned; Box2D headers and IDs stay inside the implementation. Supports body creation/destruction, box/circle shapes, transform access, stepping, body count and consuming backend-neutral Begin/End contact events keyed by engine-owned body handles.
- **Physics / PhysicsComponents2D:** serializable configuration only: PhysicsBody2D, BoxCollider2D and CircleCollider2D. The in-progress ColliderSet2D stores ordered Collider2D entries with stable nonzero ColliderIds, geometry, material, sensor and query-filter values; it clones configuration and never stores live physics handles. Runtime/serialization integration is not yet enabled.
- **Physics / Scene integration:** Scene Start builds bodies and colliders from enabled PhysicsBody2D and collider configuration. Scene Update advances a fixed 1/60 s simulation with bounded accumulator/substeps. Dynamic body transforms are synchronized back to Entity Transform2D; static/kinematic transforms are sent from entities to physics before stepping. After each fixed substep, Scene consumes PhysicsWorld contact events and maps body handles to stable EntityIds. GetPhysicsContactEvents() exposes the events collected during the most recent Update; the buffer is cleared at the start of the next valid Update. Scene query APIs convert pixel-space ray/AABB inputs to meters and map query hits back to stable EntityIds. PhysicsWorld supports closest raycast and AABB overlap queries with category/mask filters; query results are engine-owned and do not expose Box2D objects. When a transform-driven static/kinematic body moves, PhysicsWorld wakes dynamic bodies currently touching it so the next step can update contact state and emit End events. Scene Stop destroys runtime physics state.
- **Scene / SceneSerializer:** versioned JSON persistence of entity/component data; transient renderer and physics handles are never serialized.
- **Resources:** project-relative asset resolution, stable asset IDs, metadata validation and reference-counted texture lifetime.

## Scene and Play Mode
The Editor runs Play Mode on an isolated Scene clone. The authoring Scene remains unchanged when Play Mode stops. Generic components receive lifecycle callbacks through Scene Start / Update / Stop. The running clone constructs its own PhysicsWorld at Start; Stop tears it down, leaving the authoring Scene and serialized component configuration unchanged.

## Physics conventions
- Box2D v3.2.0 is a private dependency behind PhysicsWorld.
- Physics uses meters; conversion is 100 pixels = 1 meter at the Scene boundary.
- Positive Y points downward; default gravity is positive Y.
- Entity Transform2D position maps to body origin; rotation is radians.
- Collider geometry is explicit and independent of RenderComponent2D.
- The initial physics slice supported one collider per entity. Multiple colliders are the active extension milestone; see [MULTIPLE_COLLIDERS_DESIGN.md](MULTIPLE_COLLIDERS_DESIGN.md).
- A collider without PhysicsBody2D is skipped with a diagnostic.
- Dynamic bodies are physics-authoritative after stepping; static and kinematic bodies are transform-authoritative before stepping.
- Physics body handles are transient runtime state, never part of serialization or Component::Clone.
- Editor physics debug overlay visualizes configured collider geometry (orange solids, cyan sensors) without querying or exposing Box2D objects.

## Serialization rules
- Persist stable entity IDs, component type IDs, schema versions and configuration values.
- Never persist pointers, Box2D IDs, runtime body handles or renderer-owned texture handles.
- Failed scene loads should leave the destination Scene unchanged.

## Dependencies
- SDL3: windowing, events, keyboard input and rendering backend.
- SDL3_image: BMP, PNG and JPEG decoding.
- Dear ImGui: editor-only UI.
- nlohmann/json: project, asset metadata and scene serialization.
- Box2D v3.2.0: private physics backend behind PhysicsWorld.
- miniaudio: planned audio backend.

## Known gaps
- Interactive Editor/Play Mode physics verification has been completed manually on Windows; see [PHYSICS_RUNTIME_VERIFICATION.md](PHYSICS_RUNTIME_VERIFICATION.md). Multiple colliders per entity remain the active physics extension.
- Numeric body/collider configuration validation and disabled/static/kinematic transform-authority tests are implemented.
- Scene exposes closest raycast and AABB overlap queries with engine-owned results and EntityId mapping. Query category/mask filters are forwarded to PhysicsWorld. Automated tests cover excluded masks/categories, allowed query categories, and sensor inclusion in raycast and AABB overlap results.
- Runtime Play Mode input actions are not yet wired to persistent InputMap settings.
