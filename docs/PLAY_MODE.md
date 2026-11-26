# Play Mode

Play Mode runs an isolated clone of the authoring scene. Changes made while playing are discarded when Stop is pressed, leaving the saved or edited authoring scene unchanged.

## Runtime controls

- **WASD** or **Arrow keys** move the currently selected active entity.
- Movement is frame-rate independent and uses a default speed of 240 world units per second.
- Diagonal movement is normalized so it is not faster than horizontal or vertical movement.
- Select an entity in Scene Hierarchy before or during Play Mode to choose which entity receives movement.
- Press **F5** to start and **Shift+F5** to stop.

This is an initial runtime-loop proof of concept. It does not yet provide a general-purpose component update API, physics, collision, or configurable input mapping.
