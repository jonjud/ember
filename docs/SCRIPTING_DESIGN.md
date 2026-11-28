# EMBER Scripting — Design and Language Policy

## Goals
- Make C++20 the primary and first-class language for building EMBER itself and native game code.
- Allow projects to use additional gameplay scripting languages when useful, without coupling the Engine to one VM.
- Keep third-party runtime types and pointers behind a backend implementation.
- Reuse the existing Entity/Component lifecycle, InputActionState and component serialization model.
- Make script instances transient: scenes store stable project asset IDs, never VM handles.
- Translate script errors into engine-owned diagnostics; a script failure must not terminate the Editor.

## Language policy

### C++ is primary
- EMBER Engine, Editor and core systems are implemented in C++20.
- Native C++ is a first-class option for game-specific systems and performance-sensitive gameplay code.
- EMBER should provide a coherent native C++ project/build workflow rather than treating C++ as an external afterthought.
- Compiling native C++ source necessarily requires a compatible C++ compiler/toolchain on the development machine. The goal of avoiding separate installs does not remove this compiler requirement.

### Additional languages are optional and embedded
- EMBER may support one or more additional gameplay scripting languages through embedded runtimes.
- Supported runtimes should be bundled with, or automatically managed by, the EMBER Editor and exported game runtime. End users should not have to separately install a language runtime or an Editor Extension just to run a project.
- Language support should be discoverable and usable through built-in Editor/project workflows. Do not make an Extension marketplace/plugin install a prerequisite for core language support.
- Keep each language behind the same engine-owned host contract so a new backend does not require redesigning Scene, Entity or Component APIs.
- Optional language support must not compromise the C++-first architecture or force every project to ship every runtime. Runtime selection and export inclusion should be project-configurable where practical.

### Selection criteria
Do not add a concrete scripting VM dependency until candidates have been reviewed for:
1. Windows and future target-platform support.
2. Runtime size, startup cost, performance and memory behavior.
3. License compatibility and long-term maintenance.
4. Native C++ interoperability and a safe, stable binding strategy.
5. Error reporting, debugging and hot-reload possibilities.
6. Editor integration, source workflow and project packaging.
7. Whether the runtime can be bundled without separate end-user installation.

No additional language or concrete VM is selected by this document.

## First foundation slice
- `ScriptComponent` stores a project-relative script asset ID and enabled state. An empty ID is an allowed unassigned Editor state.
- `ScriptRuntime` defines a language-neutral host contract for initialization, instance creation, OnCreate, OnUpdate and OnDestroy.
- `ScriptInstanceHandle` is an opaque engine-owned integer; no VM pointer crosses the boundary.
- `ScriptExecutionContext` passes the owning Entity, frame delta and named input action state to a backend.
- ComponentRegistry owns ScriptComponent serialization and typed Inspector properties.

## Runtime lifecycle contract
1. The host initializes the selected runtime backend(s).
2. For each active, enabled ScriptComponent with a valid assigned asset, the Scene host asks the matching backend to create an instance.
3. The host invokes OnCreate once after successful instance creation.
4. Each active Scene update invokes OnUpdate with a bounded frame delta and current InputActionState.
5. Scene Stop invokes OnDestroy for each successfully created instance, then releases its handle.
6. Backend failures are reported as diagnostics and isolated to the affected script instance.

The Scene does not own VM implementation details. A backend owns source compilation/loading, VM state, instance memory, native bindings and exception translation. Native C++ game code may use a separate native build/load path, but should integrate with the same engine-owned lifecycle and diagnostics concepts where appropriate.

## Scene host integration status\n- Scene accepts a non-owning `ScriptRuntime*` at Start; the caller must keep the backend alive until Scene Stop.\n- Scene initializes and shuts down the backend, owns opaque live handles, and invokes lifecycle callbacks.\n- Only active Entities with enabled, assigned ScriptComponents create instances. Empty IDs are treated as unassigned Editor state.\n- Failed setup/update calls are recorded as Scene diagnostics; failed update instances are faulted to prevent repeated calls.\n- Runtime handles and backend pointers are transient and are not cloned or serialized.\n\n## Deliberately deferred
- Concrete optional scripting language and VM dependency selection.
- Native C++ game-code project build pipeline and hot-reload strategy.
- Script asset import/type registration and source editor.
- Script asset import/type registration and Editor assignment workflow.
- Native API bindings, Entity/Transform/physics/audio access policy and sandbox/security limits.
- Hot reload, debugging, breakpoints and packaging selected script runtimes with exported games.

These decisions are intentionally staged. Implement the Scene host lifecycle and diagnostics before selecting an embedded VM. Do not add a VM dependency to `ember_engine` until a concrete backend is selected.
