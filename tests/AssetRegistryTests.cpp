#include "Engine/Resources/AssetMetadataStore.hpp"
#include "Engine/Resources/AssetRegistry.hpp"

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

bool HasDiagnostic(
    const ember::AssetRegistry& registry,
    ember::AssetRegistryDiagnosticType type)
{
    for (const auto& diagnostic : registry.GetDiagnostics())
    {
        if (diagnostic.type == type)
        {
            return true;
        }
    }
    return false;
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "ember_asset_registry_tests";
    std::error_code filesystemError;
    std::filesystem::remove_all(root, filesystemError);
    std::filesystem::create_directories(root / "sprites", filesystemError);
    std::filesystem::create_directories(root / "textures" / "nested", filesystemError);

    std::ofstream(root / "sprites" / "player.png") << "test";
    std::ofstream(root / "textures" / "nested" / "tiles.png") << "test";

    ember::AssetMetadata player;
    player.assetId = "asset-player";
    player.source = "sprites/player.png";
    Check(ember::AssetMetadataStore::Save(
        root / "sprites" / "player.emberasset", player),
        "Test player metadata is written");

    ember::AssetMetadata tiles;
    tiles.assetId = "asset-tiles";
    tiles.source = "textures/nested/tiles.png";
    Check(ember::AssetMetadataStore::Save(
        root / "textures" / "nested" / "tiles.emberasset", tiles),
        "Nested metadata is written");

    ember::AssetMetadata missing;
    missing.assetId = "asset-missing";
    missing.source = "sprites/missing.png";
    Check(ember::AssetMetadataStore::Save(root / "missing.emberasset", missing),
        "Missing-source metadata is written");

    ember::AssetMetadata duplicate;
    duplicate.assetId = "asset-player";
    duplicate.source = "textures/nested/tiles.png";
    Check(ember::AssetMetadataStore::Save(
        root / "textures" / "duplicate.emberasset", duplicate),
        "Duplicate-ID metadata is written");

    {
        std::ofstream malformed(root / "broken.emberasset");
        malformed << "{ this is not valid JSON";
    }
    std::ofstream(root / "readme.txt") << "not a sidecar";

    ember::AssetRegistry registry(root);
    Check(registry.Scan(), "Registry scans an existing asset directory");
    Check(registry.Find("asset-tiles") != nullptr,
          "Registry finds a valid asset by stable ID");
    Check(registry.Find("asset-player") == nullptr,
          "Duplicate asset IDs are excluded instead of resolving ambiguously");
    Check(registry.Find("asset-missing") != nullptr,
          "Metadata with a missing source remains discoverable");
    Check(registry.GetAssetCount() == 2,
          "Registry indexes valid unique IDs, including missing-source entries");
    Check(HasDiagnostic(registry, ember::AssetRegistryDiagnosticType::InvalidMetadata),
          "Malformed sidecars produce diagnostics");
    Check(HasDiagnostic(registry, ember::AssetRegistryDiagnosticType::MissingSource),
          "Missing source files produce diagnostics");
    Check(HasDiagnostic(registry, ember::AssetRegistryDiagnosticType::DuplicateAssetId),
          "Duplicate IDs produce diagnostics");

    Check(registry.Scan(), "Registry can be rescanned");
    Check(registry.GetAssetCount() == 2,
          "Rescanning replaces the previous index without duplicating entries");

    ember::AssetRegistry missingRoot(root / "does-not-exist");
    Check(!missingRoot.Scan(), "Scanning a missing asset root reports failure");
    Check(HasDiagnostic(missingRoot, ember::AssetRegistryDiagnosticType::ScanError),
          "A failed root scan exposes a diagnostic");

    std::filesystem::remove_all(root, filesystemError);

    if (g_failures != 0)
    {
        std::cerr << g_failures << " asset registry test(s) failed.\n";
        return 1;
    }

    std::cout << "All asset registry tests passed.\n";
    return 0;
}
