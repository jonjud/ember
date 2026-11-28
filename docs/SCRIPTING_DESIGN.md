# EMBER Scripting — Initial Design

## Goals
- Provide gameplay scripting without exposing a specific VM or language in public Engine APIs.
- Keep third-party runtime types and pointers behind a backend implementation.
- Reuse the existing Entity/Component lifecycle, InputActionState and component serialization model.
- Make script instances transient: scenes store stable project asset IDs, never VM handles.
- Translate script errors into engine-owned diagnostics; a script failure must not terminate the Editor.

## First foundation slice
- `ScriptComponent` stores a project-relative script asset ID and enabled state. An empty ID is an allowed unassigned Editor state.
- `ScriptRuntime` defines a language-neutral host contract for initialization, instance creation, OnCreate, OnUpdate and OnDestroy.
- `ScriptInstanceHandle` is an opaque engine-owned integer; no VM pointer crosses the boundary.
- `ScriptExecutionContext` passes the owning Entity, frame delta and named input action state to a backend.
- ComponentRegistry owns ScriptComponent serialization and typed Inspector properties.

## Runtime lifecycle contract
1. The host initializes a concrete ScriptRuntime backend.
2. For each active, enabled ScriptComponent with a valid assigned asset, the Scene host asks the backend to create an instance.
3. The host invokes OnCreate once after successful instance creation.
4. Each active Scene update invokes OnUpdate with a bounded frame delta and current InputActionState.
5. Scene Stop invokes OnDestroy for each successfully created instance, then releases its handle.
6. Backend failures are reported as diagnostics and isolated to the affected script instance.

The Scene does not own VM implementation details. A backend owns source compilation/loading, VM state, instance memory, native bindings and exception translation.

## Deliberately deferred
- Choice of scripting language and concrete VM dependency.
- Script asset import/type registration and source editor.
- Scene lifecycle integration and runtime instance ownership.
- Native API bindings, Entity/Transform/physics/audio access policy and sandbox/security limits.
- Hot reload, debugging, breakpoints and packaging script runtimes with exported games.

These decisions are intentionally deferred until the host contract and use cases are reviewed. Do not add a VM dependency to `ember_engine` until a concrete backend is selected.
