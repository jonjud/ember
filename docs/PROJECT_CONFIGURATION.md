# Project Configuration

EMBER project files use the `.emberproject` extension and JSON schema version 2.

## Example

```json
{
    "schema_version": 2,
    "name": "Sample Game",
    "asset_directory": "assets",
    "startup_scene": "scenes/main.emberscene",
    "input_bindings_configured": true,
    "input_bindings": {
        "MoveUp": [26],
        "MoveDown": [22],
        "MoveLeft": [4],
        "MoveRight": [7]
    }
}
```

Input binding keys are SDL physical scancode integer values. The exact default bindings are managed by InputMap; the example illustrates the schema shape.

## Paths

Configured asset and startup-scene paths are relative to the directory containing the project file. The asset directory is required. The startup scene is optional and may be empty until a scene is selected.

Absolute paths, rooted paths, and parent-directory traversal segments are rejected. Resolution is lexical; symlink escapes are not prevented by this layer.

## Compatibility

Schema version 1 projects are migrated when loaded. New saves use schema version 2, which adds optional persisted named input bindings and a flag indicating whether the project has explicitly configured them.

## API

- `ProjectConfigurationStore::Save` validates and writes a project file.
- `ProjectConfigurationStore::Load` parses and validates the schema, including supported migration.
- `ResolveAssetDirectory` resolves the configured asset root.
- `ResolveStartupScene` resolves the configured startup scene when one is configured.

The editor applies the project asset directory, loads the configured startup scene on launch, and provides Project Settings and Scene Manager workflows.
