# EMBER Editor — Play Mode

The editor provides a safe Play / Stop preview workflow.

- Choose **Scene → Play** (F5) to clone the authoring scene and switch the hierarchy, inspector preview, and viewport to that isolated copy.
- Choose **Scene → Stop** (Shift+F5) to discard the play copy and return to the unchanged authoring scene.
- Saving, loading, and editing entity properties are disabled while Play Mode is active.
- The current milestone is an isolated scene preview. Runtime update systems, game input, physics, and scripting are not yet connected to Play Mode.

The scene clone preserves entity IDs, names, active flags, transforms, render component data, and the next-ID sequence. Entity objects are independent; renderer texture handles remain non-owning references as defined by `TextureHandle`.
