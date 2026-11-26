# Play Mode

Play Mode runs an isolated clone of the authoring scene. Changes made while playing are discarded when Stop is pressed, leaving the saved or edited authoring scene unchanged.

## Runtime controls

- Enable **Movement → Enable keyboard movement** on an entity in the Inspector before starting Play Mode.
- Select that entity in Scene Hierarchy.
- **WASD** or **Arrow keys** move the selected active entity.
- Configure its movement speed in the Inspector; the default is 240 world units per second.
- Movement is frame-rate independent and diagonal movement is normalized.
- Press **F5** to start and **Shift+F5** to stop.

Movement settings are part of the scene data and survive save/load and Play Mode cloning. Older scene files without movement settings load with movement disabled.

This is an initial runtime-loop proof of concept. It does not yet provide a general-purpose component update API, physics, collision, or configurable input mapping.
