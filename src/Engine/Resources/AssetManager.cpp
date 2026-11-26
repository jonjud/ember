#include "Engine/Resources/AssetManager.hpp"

#include <utility>

namespace ember
{
AssetManager::AssetManager(std::filesystem::path assetRoot)
    : m_directory(std::move(assetRoot))
{
}

bool AssetManager::SetTextureBackend(
    TextureCache::LoadFunction load,
    TextureCache::DestroyFunction destroy)
{
    return m_textureCache.SetBackend(std::move(load), std::move(destroy));
}

TextureHandle AssetManager::LoadTexture(const std::filesystem::path& relativePath)
{
    const std::string key = ResolveKey(relativePath);
    return key.empty() ? TextureHandle{} : m_textureCache.Load(key);
}

bool AssetManager::ReleaseTexture(const std::filesystem::path& relativePath)
{
    const std::string key = ResolveKey(relativePath);
    return !key.empty() && m_textureCache.Release(key);
}

void AssetManager::Clear()
{
    m_textureCache.Clear();
}

const AssetDirectory& AssetManager::GetDirectory() const noexcept
{
    return m_directory;
}

std::size_t AssetManager::GetCachedTextureCount() const
{
    return m_textureCache.GetCachedAssetCount();
}

std::string AssetManager::ResolveKey(const std::filesystem::path& relativePath) const
{
    const auto resolved = m_directory.Resolve(relativePath);
    return resolved ? resolved->generic_string() : std::string{};
}
} // namespace ember
