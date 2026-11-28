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

### Candidate evaluation (2026-11-28)

Candidates were compared against the criteria above:

| Candidate | Strengths | Trade-offs for EMBER | Result |
|---|---|---|---|
| **Lua 5.5.1** | Small C implementation, designed for embedding, direct C API, portable C build, broad ecosystem, MIT license | Dynamic language; host must deliberately restrict standard libraries and define a safe binding surface | **Selected as first optional backend** |
| Wren | Purpose-built embeddable VM, small C API, no dependencies beyond the C standard library | Smaller ecosystem and fewer ready-made engine/editor integrations than Lua | Good future alternative |
| ChaiScript | Header-only, C++-native binding model, BSD-3-Clause license | Couples script integration more closely to C++ templates and compiler/toolchain; larger compile-time integration surface | Not selected for first backend |
| AngelScript | C++-like syntax, embeddable VM, zlib-style permissive license | More explicit native type registration and a language closer to C++ rather than a lightweight general-purpose gameplay language | Not selected for first backend |

Official references reviewed:
- Lua embedding model, C API and language characteristics: https://www.lua.org/manual/5.5/manual.html
- Lua 5.5.1 release and source distribution: https://www.lua.org/ftp/
- Lua MIT license and commercial-use terms: https://www.lua.org/license.html
- Wren embedding model: https://wren.io/embedding/
- ChaiScript C++ integration and BSD license: https://chaiscript.com/ and https://github.com/ChaiScript/ChaiScript
- AngelScript current SDK documentation and license: https://www.angelcode.com/angelscript/sdk/docs/manual/index.html and https://www.angelcode.com/angelscript/sdk/docs/manual/doc_license.html

**Decision:** use **Lua 5.5.1** for the first optional gameplay scripting backend. It is implemented in C, is explicitly designed to be embedded, can be statically compiled into EMBER, and uses the MIT license with a required copyright notice. Lua 5.5.1 is the current 5.5 bug-fix release at the time of selection. Pin the exact version; Lua version series are not ABI-compatible, so backend upgrades must be deliberate and tested.

This is a first backend, not a commitment to Lua-only scripting. Other languages can later implement the same engine-owned ScriptRuntime interface. Lua's standard libraries must not be exposed wholesale: the backend should explicitly open only approved libraries, keep filesystem/process/package access unavailable by default, enforce per-callback instruction and memory budgets, and expose only a narrow engine-owned API. These controls are defense-in-depth, not a claim that arbitrary scripts become a fully secure sandbox.
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

## Script asset language metadata
- Script remains the language-neutral asset type. `script_language` is an optional lowercase identifier stored in the metadata sidecar; old sidecars without it remain valid and load as unassigned.
- Importing a `.lua` source tags it as `lua`. Other supported text-source imports remain language-neutral until a matching backend is available.
- A backend must reject Script assets tagged for a different language. The Lua backend only executes assets tagged `lua`.

## Lua 5.5.1 backend — implemented
- Lua is statically embedded and private to `LuaScriptRuntime`; public Engine/Scene/Component/Editor contracts use only the engine-owned ScriptRuntime abstraction.
- Each ScriptComponent instance owns an independent Lua VM state.
- Lifecycle callbacks: `OnCreate()`, `OnUpdate(deltaSeconds)` and `OnDestroy()`.
- Host API: `entity.id`, `entity.name`, Entity name/Transform getters and setters, and named `input.is_down`, `input.was_pressed` and `input.was_released` actions.
- The backend opens only approved standard libraries; filesystem/process/package/debug/coroutine access is not exposed. `string.dump` is removed and `load` is forced to text mode.
- Each instance has a memory cap and each callback has an instruction budget. Protected initialization and pcall/xpcall wrappers prevent script-level protected calls from swallowing instruction-budget failures.
- Runtime diagnostics isolate faulty instances. These controls are defense-in-depth, not a claim of a fully secure sandbox.

## Deliberately deferred
- Native C++ game-code project build pipeline and hot-reload strategy.
- Built-in source editor, language-aware validation, debugging, breakpoints and hot reload.
- Additional optional scripting backends and the runtime packaging workflow for exported games.
- Broader gameplay API policy for physics, audio, rendering, resources and UI.
- Combined Windows Editor manual verification for Lua Play Mode and Input Action Mapping.

Scene lifecycle, Script Asset workflow and the first Lua backend are implemented and pass Windows CI. Do not expose Lua types in public Engine, Scene, Component or Editor-facing contracts.