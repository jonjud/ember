# Asset Import Settings

The engine stores import options in each versioned `.emberasset` sidecar. The editor exposes the texture options through the Asset Browser.

## Editor workflow

1. Select a Texture asset in the Asset Browser.
2. Choose **Nearest** or **Linear** filtering.
3. Toggle **Premultiply Alpha** if the source texture requires it.
4. Select **Apply Settings** to persist the changes to the asset's sidecar.
5. Refresh or rescan assets before loading them into runtime systems.

The editor uses `AssetImportService` to validate the stable asset ID, reject ambiguous or non-texture entries, write the sidecar, and refresh its metadata registry. The Asset Browser then rescans its display registry.

## Runtime behavior

Changing metadata does not mutate already loaded renderer-owned texture handles. Existing references retain the settings and source path with which they were acquired. The application should call `AssetManager::ScanAssets()` before subsequent stable-ID loads so new acquisitions use the updated settings; outstanding references can still be released safely.

Only texture import settings are currently exposed in the UI. Audio and scene import workflows remain future work.
