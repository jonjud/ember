# Play Mode

Play Mode runs an isolated clone of the authoring scene. Changes made while playing are discarded when Stop is pressed, leaving the authoring scene unchanged.

## Runtime controls

- Enable **Movement → Enable keyboard movement** on an entity in the Inspector.
- Select that entity in Scene Hierarchy.
- **WASD** or **Arrow keys** move the selected active entity.
- Configure movement speed in the Inspector; the default is 240 world units per second.
- Movement is frame-rate independent and diagonal movement is normalized.
- Press **F5** to start and **Shift+F5** to stop.

Movement settings are stored in scene data and survive save/load and Play Mode cloning. Older scene files without movement settings load with movement disabled.

This is an initial runtime-loop proof of concept. It does not yet provide a general-purpose component update API, physics, or collision. Named InputMap actions and persistent editor input bindings exist, but runtime Play Mode movement is not yet driven by those configurable bindings.
