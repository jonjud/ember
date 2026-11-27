#pragma once

#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Resources/TextureCache.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace ember
{
// Project-facing asset API. Callers can use relative paths or stable asset IDs.
class AssetManager
{
public:
    explicit AssetManager(std::filesystem::path assetRoot = "assets");

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    [[nodiscard]] bool SetTextureBackend(
        TextureCache::LoadFunction load,
        TextureCache::DestroyFunction destroy);

    // Rebuilds the metadata index. Existing acquired texture references remain pinned
    // to the source path they were loaded from until released.
    [[nodiscard]] bool ScanAssets();
    [[nodiscard]] const AssetMetadata* FindAsset(const std::string& assetId) const noexcept;
    [[nodiscard]] const AssetRegistry& GetRegistry() const noexcept;

    [[nodiscard]] TextureHandle LoadTexture(const std::filesystem::path& relativePath);
    [[nodiscard]] bool ReleaseTexture(const std::filesystem::path& relativePath);
    [[nodiscard]] TextureHandle LoadTextureById(const std::string& assetId);
    [[nodiscard]] bool ReleaseTextureById(const std::string& assetId);
    void Clear();

    [[nodiscard]] const AssetDirectory& GetDirectory() const noexcept;
    [[nodiscard]] std::size_t GetCachedTextureCount() const;

private:
    struct AssetTextureReference
    {
        std::filesystem::path resolvedPath;
        TextureImportSettings settings{};
        std::size_t referenceCount = 0;
    };

    [[nodiscard]] std::string ResolveKey(const std::filesystem::path& relativePath) const;

    AssetRegistry m_registry;
    TextureCache m_textureCache;
    std::unordered_map<std::string, AssetTextureReference> m_assetTextureReferences;
};
} // namespace ember
