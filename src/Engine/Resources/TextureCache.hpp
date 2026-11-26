#pragma once

#include "Engine/Resources/TextureHandle.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>

namespace ember
{
// Shares loaded textures by normalized asset path. Each successful Load increments
// the reference count; Release decrements it and destroys the texture at zero.
class TextureCache
{
public:
    using LoadFunction = std::function<TextureHandle(const std::string&)>;
    using DestroyFunction = std::function<void(TextureHandle)>;

    TextureCache() = default;
    ~TextureCache();

    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    // Configure the renderer callbacks. The cache must be empty when changing backend.
    [[nodiscard]] bool SetBackend(LoadFunction load, DestroyFunction destroy);

    [[nodiscard]] TextureHandle Load(const std::string& filePath);
    [[nodiscard]] bool Release(const std::string& filePath);
    void Clear();

    [[nodiscard]] std::size_t GetCachedAssetCount() const;
    [[nodiscard]] std::size_t GetReferenceCount(const std::string& filePath) const;

private:
    struct Entry
    {
        TextureHandle texture{};
        std::size_t referenceCount = 0;
    };

    [[nodiscard]] static std::string NormalizePath(const std::string& filePath);

    LoadFunction m_load;
    DestroyFunction m_destroy;
    std::unordered_map<std::string, Entry> m_entries;
};
} // namespace ember
