# EMBER Engine Architecture

EMBER is a 2D-first engine written in C++20. The architecture grows in small vertical slices and keeps editor-only code separate from the engine library.

## Current targets

- `ember_engine`: runtime-facing engine library; no Dear ImGui dependency.
- `ember_editor` (output executable `ember`): SDL3 + Dear ImGui editor application.
- Test executables: focused unit tests for scene, serialization, assets, project settings, texture cache, and input mapping.

## Source layout

```text
src/
├── Editor/
│   └── EditorApplication.hpp/.cpp
├── Engine/
│   ├── Core/       Application, Time
│   ├── Math/       Vector2
│   ├── Platform/   Input, InputMap
│   ├── Project/    ProjectConfigurationStore
│   ├── Renderer/   Renderer2D, Camera2D
│   ├── Resources/  AssetDirectory, AssetManager, AssetRegistry,
│   │               AssetMetadataStore, AssetImportService, TextureCache
│   └── Scene/      Component, Entity, Scene, Transform2D, RenderComponent2D,
│                   SceneSerializer
└── main.cpp

tests/
├── AssetDirectoryTests.cpp
├── AssetImportServiceTests.cpp
├── AssetManagerTests.cpp
├── AssetMetadataTests.cpp
├── AssetRegistryTests.cpp
├── InputMapTests.cpp
├── ProjectConfigurationTests.cpp
├── SceneSerializerTests.cpp
├── SceneTests.cpp
└── TextureCacheTests.cpp
```

## Responsibilities and ownership

- **Core / Application:** runtime window, active Scene, AssetManager, and application event/update/render lifecycle.
- **Editor / EditorApplication:** editor window, SDL renderer, Dear ImGui lifecycle, panels, viewport, gizmos, scene workflow, and Play Mode.
- **Core / Time:** high-resolution frame timing and delta time.
- **Platform / Input:** keyboard state adapter.
- **Platform / InputMap:** named actions mapped to one or more SDL physical scancodes; default movement map and action management.
- **Project / ProjectConfigurationStore:** versioned .emberproject data, project-relative asset/startup paths, and persisted input binding settings.
- **Math / Vector2:** engine-owned 2D vector type.
- **Renderer / Camera2D:** world/screen conversion with camera position, zoom, and rotation.
- **Renderer / Renderer2D:** SDL-backed primitive/sprite rendering and renderer-owned texture lifetime.
- **Scene / Component:** polymorphic runtime behavior with Clone, OnCreate, OnUpdate, and OnDestroy hooks. Components are owned by entities and cloned independently. Structural changes are locked while a Scene is running.
- **Scene / ComponentRegistry:** explicit stable-ID and display-name registration for component types, with factories for editor/tool creation. Unregistered runtime-only component types are not exposed to creation tools.
- **Scene / Entity:** stable 64-bit ID, name, active state, Transform2D, RenderComponent2D, MovementComponent2D, and typed runtime components.
- **Scene / Scene:** owns entities; supports create, duplicate, destroy, lookup, iteration, independent cloning, and explicit Start / Update / Stop lifecycle dispatch.
- **Scene / SceneSerializer:** versioned JSON persistence of entity IDs and scene data. Renderer-owned texture handles are not serialized; stable asset IDs are.
- **Resources / TextureCache:** reference-counted texture sharing keyed by normalized path and import settings. Uses injected load/destroy callbacks.
- **Resources / AssetDirectory:** resolves project-relative paths and rejects lexical traversal outside the asset root.
- **Resources / AssetMetadataStore:** versioned .emberasset sidecars with stable IDs, asset type, source path, and texture import settings.
- **Resources / AssetRegistry:** scans sidecars, indexes stable IDs, and reports malformed metadata, missing files, duplicate IDs, and scan errors.
- **Resources / AssetManager:** combines validated paths, registry metadata, stable-ID texture loading, and texture-cache lifetime management.
- **Resources / AssetImportService:** validates and applies editor-facing texture import metadata.

## Scene and Play Mode

The editor supports scene creation, loading, saving, startup-scene selection, dirty-state prompts, entity duplication/deletion, and viewport transform editing. Play Mode runs an isolated clone of the authoring scene; stopping discards Play Mode changes.

The current Play Mode movement proof of concept moves the selected active entity using WASD or arrow keys when its MovementComponent2D is enabled. Movement speed is per entity and frame-rate independent. This is not yet a general component update/lifecycle system or a physics simulation.

## Asset and serialization rules

- Asset paths are project-relative and validated before loading.
- Stable asset IDs live in .emberasset metadata; runtime texture handles are transient.
- Texture import options currently include nearest/linear filtering and premultiplied alpha.
- Scene files use versioned .emberscene JSON and preserve stable entity and asset IDs.
- Failed scene loads must leave the destination Scene unchanged.
- Texture handles are non-owning references; release acquired cache/manager references only after scene objects no longer need them.
- Lexical path validation does not prevent filesystem symlink escapes.

## Dependencies

- SDL3: windowing, events, keyboard input, and initial rendering backend.
- SDL3_image: BMP, PNG, and JPEG decoding.
- Dear ImGui: editor-only UI dependency.
- nlohmann/json: project, asset metadata, and scene serialization.
- Box2D: planned physics backend.
- miniaudio: planned audio backend.

## Known gaps

- Interactive runtime behavior has not yet been independently verified on Windows.
- The component type registry is in place, but Inspector integration and generic component serialization are not implemented yet; physics/collision, animation, audio, UI system, and scripting interface are also not implemented yet.
- Runtime Play Mode movement is still a focused proof of concept; editor input-binding persistence is not yet wired into runtime action polling.
- The Windows workflow currently compiles the project and runs CTest; it does not validate visual editor behavior.
