#include "Engine/Resources/AssetImportService.hpp"
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
    const auto root = std::filesystem::temp_directory_path() / "ember_asset_import_service_tests";
    std::error_code filesystemError;
    std::filesystem::remove_all(root, filesystemError);
    std::filesystem::create_directories(root / "sprites", filesystemError);
    std::ofstream(root / "sprites" / "player.png") << "image";
    std::ofstream(root / "audio.ogg") << "audio";

    ember::AssetMetadata texture;
    texture.assetId = "asset-player";
    texture.source = "sprites/player.png";
    Check(ember::AssetMetadataStore::Save(
        root / "sprites" / "player.emberasset", texture),
        "Texture metadata is prepared");

    ember::AssetMetadata audio;
    audio.assetId = "asset-audio";
    audio.type = ember::AssetType::Audio;
    audio.source = "audio.ogg";
    Check(ember::AssetMetadataStore::Save(root / "audio.emberasset", audio),
          "Audio metadata is prepared");

    ember::AssetImportService importer(root);
    Check(importer.Scan(), "Import service scans asset metadata");

    ember::TextureImportSettings settings;
    settings.filter = ember::TextureFilter::Linear;
    settings.premultiplyAlpha = true;
    std::string error;
    Check(importer.UpdateTextureSettings("asset-player", settings, &error),
          "Texture import settings can be updated by stable asset ID");
    Check(error.empty(), "Successful settings update clears the error message");

    const auto updated = importer.FindAsset("asset-player");
    Check(updated != nullptr, "Updated texture remains indexed");
    if (updated != nullptr)
    {
        Check(updated->texture.filter == ember::TextureFilter::Linear,
              "Updated filter is visible in the refreshed registry");
        Check(updated->texture.premultiplyAlpha,
              "Updated alpha setting is visible in the refreshed registry");
    }

    const auto persisted = ember::AssetMetadataStore::Load(
        root / "sprites" / "player.emberasset", &error);
    Check(persisted.has_value(), "Updated settings remain persisted in the sidecar");
    if (persisted)
    {
        Check(persisted->texture.filter == ember::TextureFilter::Linear &&
              persisted->texture.premultiplyAlpha,
              "Sidecar stores both updated texture import settings");
    }

    Check(!importer.UpdateTextureSettings("missing", settings, &error),
          "Unknown asset IDs are rejected");
    Check(!importer.UpdateTextureSettings("asset-audio", settings, &error),
          "Non-texture assets reject texture import settings");

    const auto duplicateRoot = root / "duplicate";
    std::filesystem::create_directories(duplicateRoot, filesystemError);
    ember::AssetMetadata duplicate = texture;
    duplicate.source = "sprites/player.png";
    Check(ember::AssetMetadataStore::Save(
        duplicateRoot / "player-copy.emberasset", duplicate),
        "Duplicate-ID metadata fixture is prepared");
    Check(importer.Scan(), "Registry scan completes while reporting duplicate IDs");
    Check(importer.FindAsset("asset-player") == nullptr,
          "Duplicate stable IDs are excluded from lookup");
    Check(!importer.UpdateTextureSettings("asset-player", settings, &error),
          "Ambiguous duplicate IDs cannot be edited");

    std::filesystem::remove_all(root, filesystemError);

    if (g_failures != 0)
    {
        std::cerr << g_failures << " asset import service test(s) failed.\n";
        return 1;
    }

    std::cout << "All asset import service tests passed.\n";
    return 0;
}
