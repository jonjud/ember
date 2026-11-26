#include "Engine/Resources/AssetMetadataStore.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
int g_failures = 0;

void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << description << '\n';
        ++g_failures;
    }
    else
    {
        std::cout << "[PASS] " << description << '\n';
    }
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "ember_asset_metadata_tests";
    std::error_code filesystemError;
    std::filesystem::remove_all(root, filesystemError);
    std::filesystem::create_directories(root, filesystemError);

    const auto metadataPath = root / "sprites" / "player.emberasset";
    ember::AssetMetadata metadata;
    metadata.assetId = "7f2f13d8-ember-player-sprite";
    metadata.type = ember::AssetType::Texture;
    metadata.source = "sprites/player.png";
    metadata.texture.filter = ember::TextureFilter::Nearest;
    metadata.texture.premultiplyAlpha = true;

    std::string error;
    Check(ember::AssetMetadataStore::Save(metadataPath, metadata, &error),
          "Asset metadata is saved as a JSON sidecar");
    Check(error.empty(), "Successful save clears the error message");

    const auto loaded = ember::AssetMetadataStore::Load(metadataPath, &error);
    Check(loaded.has_value(), "Saved metadata can be loaded");
    if (loaded)
    {
        Check(loaded->assetId == metadata.assetId, "Stable asset ID survives round-trip");
        Check(loaded->type == ember::AssetType::Texture, "Asset type survives round-trip");
        Check(loaded->source == metadata.source, "Project-relative source survives round-trip");
        Check(loaded->texture.filter == ember::TextureFilter::Nearest,
              "Texture filter import setting survives round-trip");
        Check(loaded->texture.premultiplyAlpha,
              "Texture alpha import setting survives round-trip");
    }

    metadata.source = "../outside.png";
    Check(!ember::AssetMetadataStore::Save(root / "invalid.emberasset", metadata, &error),
          "Saving metadata rejects source paths escaping the asset root");

    metadata.source = "sprites/player.png";
    metadata.assetId.clear();
    Check(!ember::AssetMetadataStore::Save(root / "missing-id.emberasset", metadata, &error),
          "Saving metadata rejects an empty stable asset ID");

    const auto malformedPath = root / "unsupported.emberasset";
    {
        std::ofstream malformed(malformedPath);
        malformed << R"({"schema_version":99,"asset_id":"x","type":"texture","source":"x.png","texture":{"filter":"nearest","premultiply_alpha":false}})";
    }
    Check(!ember::AssetMetadataStore::Load(malformedPath, &error),
          "Loading rejects unsupported metadata schema versions");

    std::filesystem::remove_all(root, filesystemError);

    if (g_failures != 0)
    {
        std::cerr << g_failures << " asset metadata test(s) failed.\n";
        return 1;
    }

    std::cout << "All asset metadata tests passed.\n";
    return 0;
}
