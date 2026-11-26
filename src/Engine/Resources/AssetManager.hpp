#pragma once

#include "Engine/Resources/AssetDirectory.hpp"
#include "Engine/Resources/TextureCache.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

namespace ember
{
// Project-facing asset API. Callers use paths relative to the configured asset root.
class AssetManager
{
public:
    explicit AssetManager(std::filesystem::path assetRoot = "assets");

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    [[nodiscard]] bool SetTextureBackend(
        TextureCache::LoadFunction load,
        TextureCache::DestroyFunction destroy);

    [[nodiscard]] TextureHandle LoadTexture(const std::filesystem::path& relativePath);
    [[nodiscard]] bool ReleaseTexture(const std::filesystem::path& relativePath);
    void Clear();

    [[nodiscard]] const AssetDirectory& GetDirectory() const noexcept;
    [[nodiscard]] std::size_t GetCachedTextureCount() const;

private:
    [[nodiscard]] std::string ResolveKey(const std::filesystem::path& relativePath) const;

    AssetDirectory m_directory;
    TextureCache m_textureCache;
};
} // namespace ember
