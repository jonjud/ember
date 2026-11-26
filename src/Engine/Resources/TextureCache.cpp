#include "Engine/Resources/TextureCache.hpp"

#include <filesystem>
#include <limits>
#include <utility>

namespace ember
{
TextureCache::~TextureCache()
{
    Clear();
}

bool TextureCache::SetBackend(LoadFunction load, DestroyFunction destroy)
{
    if (!m_entries.empty() || !load || !destroy)
    {
        return false;
    }

    m_load = std::move(load);
    m_destroy = std::move(destroy);
    return true;
}

TextureHandle TextureCache::Load(const std::string& filePath)
{
    if (!m_load || !m_destroy)
    {
        return {};
    }

    const std::string key = NormalizePath(filePath);
    if (key.empty())
    {
        return {};
    }

    const auto existing = m_entries.find(key);
    if (existing != m_entries.end())
    {
        if (existing->second.referenceCount == std::numeric_limits<std::size_t>::max())
        {
            return {};
        }

        ++existing->second.referenceCount;
        return existing->second.texture;
    }

    const TextureHandle texture = m_load(key);
    if (!texture)
    {
        return {};
    }

    m_entries.emplace(key, Entry{texture, 1});
    return texture;
}

bool TextureCache::Release(const std::string& filePath)
{
    const std::string key = NormalizePath(filePath);
    const auto existing = m_entries.find(key);
    if (existing == m_entries.end())
    {
        return false;
    }

    if (existing->second.referenceCount > 1)
    {
        --existing->second.referenceCount;
        return true;
    }

    if (m_destroy)
    {
        m_destroy(existing->second.texture);
    }
    m_entries.erase(existing);
    return true;
}

void TextureCache::Clear()
{
    if (m_destroy)
    {
        for (const auto& [path, entry] : m_entries)
        {
            (void)path;
            m_destroy(entry.texture);
        }
    }
    m_entries.clear();
}

std::size_t TextureCache::GetCachedAssetCount() const
{
    return m_entries.size();
}

std::size_t TextureCache::GetReferenceCount(const std::string& filePath) const
{
    const auto existing = m_entries.find(NormalizePath(filePath));
    return existing == m_entries.end() ? 0 : existing->second.referenceCount;
}

std::string TextureCache::NormalizePath(const std::string& filePath)
{
    if (filePath.empty())
    {
        return {};
    }

    return std::filesystem::path(filePath).lexically_normal().generic_string();
}
} // namespace ember
