#pragma once

#include "Engine/Resources/AssetMetadata.hpp"
#include "Engine/Resources/TextureHandle.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>

namespace ember
{
class TextureCache
{
public:
    using LoadFunction = std::function<TextureHandle(
        const std::string&, const TextureImportSettings&)>;
    using DestroyFunction = std::function<void(TextureHandle)>;

    TextureCache() = default;
    ~TextureCache();

    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    [[nodiscard]] bool SetBackend(LoadFunction load, DestroyFunction destroy);
    [[nodiscard]] TextureHandle Load(
        const std::string& filePath,
        const TextureImportSettings& settings = {});
    [[nodiscard]] bool Release(
        const std::string& filePath,
        const TextureImportSettings& settings = {});
    void Clear();

    [[nodiscard]] std::size_t GetCachedAssetCount() const;
    [[nodiscard]] std::size_t GetReferenceCount(
        const std::string& filePath,
        const TextureImportSettings& settings = {}) const;

private:
    struct Entry
    {
        TextureHandle texture{};
        std::size_t referenceCount = 0;
    };

    [[nodiscard]] static std::string MakeKey(
        const std::string& filePath,
        const TextureImportSettings& settings);

    LoadFunction m_load;
    DestroyFunction m_destroy;
    std::unordered_map<std::string, Entry> m_entries;
};
} // namespace ember
