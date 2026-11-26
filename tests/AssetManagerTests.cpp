#include "Engine/Resources/AssetManager.hpp"
#include "Engine/Resources/AssetMetadataStore.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

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
    const auto root = std::filesystem::temp_directory_path() / "ember_asset_manager_tests";
    std::error_code filesystemError;
    std::filesystem::remove_all(root, filesystemError);
    std::filesystem::create_directories(root / "sprites", filesystemError);
    std::ofstream(root / "sprites" / "player.bmp") << "test";

    ember::AssetMetadata playerMetadata;
    playerMetadata.assetId = "asset-player";
    playerMetadata.source = "sprites/player.bmp";
    Check(ember::AssetMetadataStore::Save(
        root / "sprites" / "player.emberasset", playerMetadata),
        "Player metadata is written");

    ember::AssetMetadata audioMetadata;
    audioMetadata.assetId = "asset-audio";
    audioMetadata.type = ember::AssetType::Audio;
    audioMetadata.source = "audio/theme.ogg";
    Check(ember::AssetMetadataStore::Save(
        root / "theme.emberasset", audioMetadata),
        "Audio metadata is written");

    ember::AssetManager assets(root);
    std::unordered_map<std::uint32_t, int> destroyed;
    std::uint32_t nextId = 1;
    int loadCalls = 0;

    Check(assets.SetTextureBackend(
        [&nextId, &loadCalls](const std::string& path)
        {
            ++loadCalls;
            Check(path.find("sprites/player.bmp") != std::string::npos,
                  "Backend receives the resolved project asset path");
            return ember::TextureHandle{nextId++};
        },
        [&destroyed](ember::TextureHandle handle)
        {
            ++destroyed[handle.id];
        }), "Asset manager accepts texture backend callbacks");

    Check(assets.ScanAssets(), "Asset manager scans project metadata");
    Check(assets.FindAsset("asset-player") != nullptr,
          "Asset manager finds metadata by stable ID");

    const auto player = assets.LoadTextureById("asset-player");
    const auto samePlayer = assets.LoadTextureById("asset-player");
    Check(player && samePlayer && player.id == samePlayer.id,
          "Stable ID loads the same cached texture");
    Check(loadCalls == 1, "Repeated stable-ID requests load the texture once");
    Check(assets.GetCachedTextureCount() == 1,
          "Stable-ID texture is tracked by the shared cache");

    Check(!assets.LoadTextureById("unknown-id"),
          "Unknown stable IDs cannot be loaded");
    Check(!assets.LoadTextureById("asset-audio"),
          "Non-texture assets cannot be loaded through the texture API");
    Check(loadCalls == 1, "Invalid stable IDs never reach the renderer backend");

    // A rescan that no longer contains the ID must not break release of an
    // already acquired reference: the manager pins the original resolved path.
    std::filesystem::remove(root / "sprites" / "player.emberasset", filesystemError);
    Check(assets.ScanAssets(), "Registry can be rescanned after metadata changes");
    Check(assets.FindAsset("asset-player") == nullptr,
          "Rescan removes metadata that is no longer present");
    Check(assets.ReleaseTextureById("asset-player"),
          "Previously acquired ID reference can be released after a rescan");
    Check(destroyed[player.id] == 0,
          "Texture remains alive while another stable-ID reference exists");
    Check(assets.ReleaseTextureById("asset-player"),
          "Final stable-ID reference can be released after a rescan");
    Check(destroyed[player.id] == 1,
          "Texture is destroyed after the final stable-ID release");
    Check(!assets.ReleaseTextureById("asset-player"),
          "Releasing an unacquired stable ID fails safely");
    Check(assets.GetCachedTextureCount() == 0,
          "Released stable-ID texture is removed from cache");

    const auto byPath = assets.LoadTexture("sprites/player.bmp");
    Check(static_cast<bool>(byPath), "Path-based loading remains available");
    Check(assets.ReleaseTexture("sprites/player.bmp"),
          "Path-based release remains available");
    Check(!assets.LoadTexture("../outside.bmp"),
          "Asset manager rejects paths escaping the project asset directory");
    Check(!assets.LoadTexture(""),
          "Asset manager rejects empty asset paths");

    std::filesystem::remove_all(root, filesystemError);

    if (g_failures != 0)
    {
        std::cerr << g_failures << " asset manager test(s) failed.\n";
        return 1;
    }

    std::cout << "All asset manager tests passed.\n";
    return 0;
}
