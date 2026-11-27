# EMBER Engine Architecture

EMBER is a 2D-first engine written in C++20. It grows through small vertical slices and keeps editor-only code separate from the engine library.

## Current targets
- `ember_engine`: runtime-facing engine library; no Dear ImGui dependency.
- `ember_editor` (output executable `ember`): SDL3 + Dear ImGui editor.
- Focused test executables run through CTest in Windows CI.

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
└── (other focused engine tests)
```

## Responsibilities and ownership
- **Core / Application:** runtime window, active Scene, AssetManager, and application event/update/render lifecycle.
- **Editor / EditorApplication:** editor panels, viewport, gizmos, scene workflow and Play Mode; editor UI does not own physics backend objects.
- **Math / Vector2:** engine-owned 2D vector type.
- **Scene / Component:** polymorphic runtime behavior with Clone, OnCreate, OnUpdate and OnDestroy hooks. Components are owned by entities; structural changes are locked while a Scene is running.
- **Scene / ComponentRegistry:** stable IDs, factories, versioned JSON serializers, schema migrations and UI-agnostic typed property descriptors. Current Inspector types include bool, int, float, string, Vector2 and Enumeration; the Editor maps these descriptors to ImGui controls.
- **Scene / Entity:** stable 64-bit ID, name, active state, Transform2D, render/movement data and registered runtime components.
- **Scene / Scene:** owns entities; supports create, duplicate, destroy, lookup, iteration, cloning and Start / Update / Stop lifecycle dispatch.
- **Physics / PhysicsWorld:** EMBER-owned RAII facade around Box2D v3.2.0. Public descriptors and handles are engine-owned; Box2D headers and IDs stay inside the implementation. Supports body creation/destruction, box/circle shapes, transform access, stepping and body count.
- **Physics / PhysicsComponents2D:** serializable configuration only: PhysicsBody2D, BoxCollider2D and CircleCollider2D. They clone configuration and never store live physics handles.
- **Physics / Scene integration (next):** a running Scene will own PhysicsWorld and runtime entity-to-body mapping; physics state is built at Start, stepped at fixed timestep and destroyed at Stop. Dynamic transforms will be synchronized back to Entity Transform2D.
- **Scene / SceneSerializer:** versioned JSON persistence of entity/component data; transient renderer and physics handles are never serialized.
- **Resources:** project-relative asset resolution, stable asset IDs, metadata validation and reference-counted texture lifetime.

## Scene and Play Mode
The Editor runs Play Mode on an isolated Scene clone. The authoring Scene remains unchanged when Play Mode stops. Generic components receive lifecycle callbacks through Scene Start / Update / Stop. Physics runtime integration is the next planned layer; Physics components currently represent editable configuration and do not yet simulate.

## Physics conventions
- Box2D v3.2.0 is a private dependency of `ember_engine`.
- Physics uses meters; conversion is 100 pixels = 1 meter at the Scene boundary.
- Positive Y points downward; default gravity is positive Y.
- Entity Transform2D position maps to body origin; rotation is radians.
- Collider geometry is explicit and independent of RenderComponent2D.
- One collider per entity in the first vertical slice; ambiguous multiple collider types should be rejected with diagnostics.
- Physics body handles are transient runtime state, never part of serialization or Component::Clone.

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
- PhysicsWorld is implemented and tested, but Scene does not yet create bodies or step simulation.
- Physics component Inspector supports editable configuration; changes do not affect simulation until Scene runtime integration lands.
- Interactive runtime behavior still requires manual verification on Windows.
- Runtime Play Mode input actions are not yet wired to persistent InputMap settings.
