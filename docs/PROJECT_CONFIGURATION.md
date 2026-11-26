# Project Configuration

EMBER project files use the `.emberproject` extension and JSON schema version 1.

## Example

```json
{
    "schema_version": 1,
    "name": "Sample Game",
    "asset_directory": "assets",
    "startup_scene": "scenes/main.emberscene"
}
```

## Paths

Both configured paths are relative to the directory containing the project file. The asset directory is required. The startup scene is optional and may be an empty string until a scene is selected.

Absolute paths, rooted paths, and parent-directory traversal segments (`..`) are rejected. Resolution is lexical; symlink escapes are not prevented by this layer.

## API

- `ProjectConfigurationStore::Save` validates and writes a project file.
- `ProjectConfigurationStore::Load` parses and validates the schema.
- `ResolveAssetDirectory` resolves the configured asset root.
- `ResolveStartupScene` resolves the startup scene when one is configured.

This layer describes project settings only. It does not yet open the scene or create editor UI; those responsibilities belong to later application/editor integration work.
