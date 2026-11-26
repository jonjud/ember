# Asset Import Settings Service

The AssetImportService is the editor-facing API for changing persisted import metadata without coupling the editor to the runtime renderer.

## Workflow

1. Construct the service with the project's asset directory.
2. Call Scan() to index .emberasset sidecars.
3. Read an asset with FindAsset(stableAssetId).
4. For texture assets, call UpdateTextureSettings(stableAssetId, settings).
5. The service writes the updated sidecar and refreshes its own registry.

The registry retains the sidecar path for each unique stable asset ID. If duplicate IDs are found, the ID is removed from lookup and cannot be edited through this API.

## Runtime behavior

The service does not mutate AssetManager or renderer-owned texture handles. Existing runtime references keep the import settings and source path with which they were acquired. After editing settings, the application should call AssetManager::ScanAssets() before subsequent stable-ID loads; those later loads use the new metadata while outstanding references can still be released safely.

This separation allows a future editor Inspector to expose filter and alpha options without making the runtime asset lifetime dependent on editor state.
