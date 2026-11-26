# EMBER Engine Architecture

EMBER is a 2D-first engine written in C++20. The architecture is intentionally small and grows only when a working use case requires another subsystem.

## Current executable

The `ember` executable is the application entry point. It creates an `ember::Application` and delegates initialization, the main loop, and shutdown to the engine library.

## Current source layout

```text
src/
├── Engine/
│   ├── Core/
│   │   ├── Application.hpp/.cpp
│   │   └── Time.hpp/.cpp
│   ├── Math/
│   │   └── Vector2.hpp
│   ├── Platform/
│   │   └── Input.hpp/.cpp
│   ├── Renderer/
│   │   ├── Camera2D.hpp/.cpp
│   │   └── Renderer2D.hpp/.cpp
│   ├── Resources/
│   │   ├── AssetDirectory.hpp/.cpp
│   │   ├── AssetManager.hpp/.cpp
│   │   ├── AssetMetadata.hpp
│   │   ├── AssetMetadataStore.hpp/.cpp
│   │   ├── AssetRegistry.hpp/.cpp
│   │   ├── TextureCache.hpp/.cpp
│   │   └── TextureHandle.hpp
│   └── Scene/
│       ├── Entity.hpp
│       ├── RenderComponent2D.hpp
│       ├── Scene.hpp/.cpp
│       ├── SceneSerializer.hpp/.cpp
│       └── Transform2D.hpp
└── main.cpp

tests/
├── AssetDirectoryTests.cpp
├── AssetManagerTests.cpp
├── AssetMetadataTests.cpp
├── AssetRegistryTests.cpp
├── SceneTests.cpp
├── SceneSerializerTests.cpp
└── TextureCacheTests.cpp
```

## Current responsibilities

- **Core / Application:** owns the SDL window, active scene, project-facing AssetManager, and event/update/render loop.
- **Core / Time:** measures frame delta time using SDL's high-resolution performance counter.
- **Platform / Input:** exposes keyboard state through an engine-facing API.
- **Math / Vector2:** engine-owned 2D vector type, independent of SDL.
- **Resources / TextureHandle:** small, renderer-independent identifier for a texture resource. A handle is non-owning and is valid only while its owning renderer retains the resource.
- **Resources / TextureCache:** shares texture handles by normalized file path and import settings, counts active references, and destroys a texture when its final reference is released. Different filter/alpha settings create distinct cached variants. It uses injected load/destroy callbacks, keeping SDL out of the resource cache and allowing deterministic unit tests.
- **Resources / AssetDirectory:** resolves project-relative asset paths and rejects empty, absolute, and lexically escaping paths.
- **Resources / AssetManager:** project-facing façade that scans the AssetRegistry, resolves stable texture asset IDs, and combines validated paths with the reference-counted texture cache. Stable-ID acquisitions pin their resolved source path until all references are released, even if metadata is rescanned.
- **Resources / AssetMetadata:** stores a stable project asset ID, asset type, source path, and texture import settings. Runtime texture handles are never persisted.
- **Resources / AssetMetadataStore:** reads and writes versioned JSON sidecar files, rejects unsupported schema versions, and validates project-relative source paths.
- **Resources / AssetRegistry:** recursively scans `.emberasset` sidecars, indexes valid metadata by stable ID, and reports malformed metadata, missing source files, duplicate IDs, and scan errors. Ambiguous duplicate IDs are excluded from lookup.
- **Scene / Entity:** provides a stable 64-bit entity ID, name, active state, Transform2D, and RenderComponent2D.
- **Scene / Scene:** owns entities, creates and destroys them, looks them up by ID, and iterates without allocating a temporary list. Entity references remain stable when other entities are inserted; destroying that entity invalidates its reference.
- **Scene / SceneSerializer:** saves and loads versioned JSON scenes, preserving entity IDs and scene data while storing stable texture asset IDs instead of transient renderer handles. Failed loads leave the destination scene unchanged.
- **Scene / Transform2D:** stores an object's world position, scale, and rotation in radians.
- **Scene / RenderComponent2D:** stores visual size, RGBA tint, visibility, a stable texture asset ID, and an optional runtime texture handle. Without a valid handle it draws a filled rectangle; with a valid handle it draws a sprite.
- **Renderer / Camera2D:** converts world positions and sizes into screen coordinates and applies camera zoom and rotation.
- **Renderer / Renderer2D:** owns the SDL renderer, draws primitives and sprites, and manages loaded texture lifetimes.
- **main.cpp:** contains only the process entry point.

## Scene and entity workflow

- Construct a `Scene`, optionally providing its name.
- Call `CreateEntity(name)` to create an entity and receive a stable reference.
- Read or update its transform through `GetTransform()`.
- Configure its visual through `GetRenderComponent()`; set `texture` to a valid `TextureHandle` for a sprite or leave it empty for a filled rectangle.
- Use `GetId()` to retain its identity and `FindEntity(id)` to look it up later.
- Call `DestroyEntity(id)` to remove one entity or `Clear()` to remove all entities.
- Use `ForEachEntity(callback)` to update or render entities without allocating a temporary list.
- Entity IDs are monotonically assigned and are not reused during a scene's lifetime. ID exhaustion is reported with `std::overflow_error`.

## Asset directory workflow

- Construct an `AssetDirectory` with the project's asset root, such as `assets/`.
- Call `Resolve(relativePath)` before passing an asset path to a loader.
- Absolute paths, empty paths, and paths that lexically escape the root are rejected.
- The resolver performs lexical checks only; projects that allow untrusted asset folders should additionally enforce filesystem-level symlink policy.

## Asset manager workflow

- Construct an `AssetManager` with the project's asset folder (defaults to `assets/`).
- Connect its texture backend after the renderer is initialized.
- Use `LoadTexture("sprites/player.png")` with a path relative to the asset folder; BMP, PNG, and JPEG files are supported.
- Call `ReleaseTexture` once for each successful load when no component needs that reference anymore.
- Call `Clear()` before shutting down the renderer.

## Texture cache workflow

- Configure the cache with a loader and a destroy callback once the renderer is initialized.
- Call `Load(path)` to load an asset or acquire another reference to an already loaded asset. Paths are lexically normalized, so equivalent relative paths share a cache entry.
- Assign the returned handle to a component. Components borrow handles; they do not own cache references.
- Call `Release(path)` once for every successful `Load(path)`. The underlying texture is destroyed when the reference count reaches zero.
- Call `Clear()` before shutting down the renderer to release every remaining texture.
- A failed load returns an empty handle and does not create a cache entry.

The cache uses callbacks rather than SDL types. The AssetManager connects the validated project-relative path workflow to the cache. The Application clears the cache before destroying the renderer. Do not release a path while an entity still intends to draw its handle.

## Scene serialization workflow

- Save scenes as versioned `.emberscene` JSON files using `SceneSerializer::Save`.
- Load with `SceneSerializer::Load`; the destination is replaced only after the complete file has been validated.
- Entity IDs, names, active states, transforms, render settings, and stable texture asset IDs are persisted.
- Renderer-owned `TextureHandle` values are never serialized. After loading, resolve each texture asset ID through the asset system and acquire a fresh runtime handle.
- Unknown schema versions, duplicate/zero entity IDs, malformed values, and invalid numeric ranges are rejected.

## Asset metadata workflow

- Each imported asset can have a JSON sidecar with the `.emberasset` extension.
- Metadata stores a stable asset ID, asset type, project-relative source path, and texture import settings such as nearest/linear filtering and alpha premultiplication. Stable-ID texture loads pass these settings into Renderer2D; nearest/linear filtering selects the SDL texture scale mode, and premultiplied alpha converts source pixels to premultiplied RGBA and uses premultiplied blending.
- Metadata schema versioning allows the format to evolve without silently accepting incompatible files.
- Runtime handles are intentionally excluded; they are transient renderer-owned values and must be reacquired when a project loads.

## Renderer texture workflow

- `Renderer2D::LoadTexture(path)` decodes BMP, PNG, and JPEG images through SDL3_image and returns an engine-owned `TextureHandle`.
- `Renderer2D::DestroyTexture(handle)` releases an individual texture. Prefer the AssetManager for shared project assets.
- Camera-aware sprite drawing applies object position, scale, rotation, camera conversion, and per-component RGBA tint.
- Texture handles are lightweight, non-owning IDs; SDL texture pointers remain private to the renderer.
- Image decoding is isolated behind the existing `Renderer2D::LoadTexture` interface.

## Planned layers

```text
Editor
  └── Engine Runtime
        ├── Core       (application, time, logging)
        ├── Platform   (window, input)
        ├── Renderer   (2D drawing, textures, camera)
        ├── Scene      (entities, components, serialization)
        ├── Physics    (2D collision and simulation)
        ├── Audio
        └── Resources  (asset loading and lifetime)
```

## Dependency policy

- SDL3: windowing, event handling, keyboard input, and initial rendering backend.
- SDL3_image: BMP, PNG, and JPEG image decoding (stb_image backend).
- Dear ImGui: planned editor interface; not part of the runtime core.
- Box2D: planned 2D physics backend.
- miniaudio: planned audio backend.

Third-party APIs should be hidden behind narrow engine-facing interfaces when doing so provides a concrete portability, testing, or maintenance benefit.

## Asset registry workflow

- Construct an `AssetRegistry` with the project's asset root and call `Scan()` to rebuild its index.
- `Find(assetId)` returns metadata for a unique stable ID; IDs duplicated across sidecars are deliberately excluded.
- `GetDiagnostics()` reports malformed sidecars, missing source files, duplicate IDs, and directory scan errors.
- Metadata whose source file is missing remains indexed so tools can display and repair the broken asset reference.
- The registry is independent of the renderer; it does not create runtime texture handles or silently load assets.

## Next milestone

Add editor-facing asset import settings and begin the project configuration/editor foundation.

\n## Stable asset ID loading workflow\n\n- Construct an \`AssetManager\` with the project's asset root and configure its texture backend after renderer initialization.\n- Call \`ScanAssets()\` to populate its metadata registry from \`.emberasset\` sidecars.\n- Use \`FindAsset(assetId)\` to inspect metadata or \`LoadTextureById(assetId)\` to acquire a texture by stable ID.\n- Call \`ReleaseTextureById(assetId)\` once for each successful stable-ID load. The manager remembers the original resolved source path, so a registry rescan does not redirect or leak outstanding references.\n- Only assets whose metadata type is \`Texture\` can be loaded through the texture API. Unknown IDs and other asset types return an empty handle.\n- Path-based \`LoadTexture\` and \`ReleaseTexture\` remain available for runtime and migration workflows.\n