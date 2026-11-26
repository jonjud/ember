#include "Engine/Resources/AssetManager.hpp"

#include <limits>
#include <utility>

namespace ember
{
AssetManager::AssetManager(std::filesystem::path assetRoot)
    : m_registry(std::move(assetRoot))
{
}

bool AssetManager::SetTextureBackend(
    TextureCache::LoadFunction load,
    TextureCache::DestroyFunction destroy)
{
    return m_textureCache.SetBackend(std::move(load), std::move(destroy));
}

bool AssetManager::ScanAssets()
{
    return m_registry.Scan();
}

const AssetMetadata* AssetManager::FindAsset(const std::string& assetId) const noexcept
{
    return m_registry.Find(assetId);
}

const AssetRegistry& AssetManager::GetRegistry() const noexcept
{
    return m_registry;
}

TextureHandle AssetManager::LoadTexture(const std::filesystem::path& relativePath)
{
    const std::string key = ResolveKey(relativePath);
    return key.empty() ? TextureHandle{} : m_textureCache.Load(key, {});
}

bool AssetManager::ReleaseTexture(const std::filesystem::path& relativePath)
{
    const std::string key = ResolveKey(relativePath);
    return !key.empty() && m_textureCache.Release(key, {});
}

TextureHandle AssetManager::LoadTextureById(const std::string& assetId)
{
    if (assetId.empty())
    {
        return {};
    }

    auto reference = m_assetTextureReferences.find(assetId);
    if (reference == m_assetTextureReferences.end())
    {
        const AssetMetadata* metadata = m_registry.Find(assetId);
        if (metadata == nullptr || metadata->type != AssetType::Texture)
        {
            return {};
        }

        const auto resolved = m_registry.GetDirectory().Resolve(metadata->source);
        if (!resolved)
        {
            return {};
        }

        const std::string key = resolved->generic_string();
        const TextureHandle texture = m_textureCache.Load(key, metadata->texture);
        if (!texture)
        {
            return {};
        }

        m_assetTextureReferences.emplace(
            assetId, AssetTextureReference{*resolved, metadata->texture, 1});
        return texture;
    }

    if (reference->second.referenceCount == std::numeric_limits<std::size_t>::max())
    {
        return {};
    }

    const TextureHandle texture =
        m_textureCache.Load(reference->second.resolvedPath.generic_string(), reference->second.settings);
    if (!texture)
    {
        return {};
    }

    ++reference->second.referenceCount;
    return texture;
}

bool AssetManager::ReleaseTextureById(const std::string& assetId)
{
    const auto reference = m_assetTextureReferences.find(assetId);
    if (reference == m_assetTextureReferences.end() ||
        reference->second.referenceCount == 0)
    {
        return false;
    }

    if (!m_textureCache.Release(reference->second.resolvedPath.generic_string(), reference->second.settings))
    {
        return false;
    }

    --reference->second.referenceCount;
    if (reference->second.referenceCount == 0)
    {
        m_assetTextureReferences.erase(reference);
    }
    return true;
}

void AssetManager::Clear()
{
    m_textureCache.Clear();
    m_assetTextureReferences.clear();
}

const AssetDirectory& AssetManager::GetDirectory() const noexcept
{
    return m_registry.GetDirectory();
}

std::size_t AssetManager::GetCachedTextureCount() const
{
    return m_textureCache.GetCachedAssetCount();
}

std::string AssetManager::ResolveKey(const std::filesystem::path& relativePath) const
{
    const auto resolved = m_registry.GetDirectory().Resolve(relativePath);
    return resolved ? resolved->generic_string() : std::string{};
}
} // namespace ember
