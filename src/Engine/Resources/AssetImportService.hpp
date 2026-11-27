#pragma once

#include "Engine/Resources/AssetRegistry.hpp"

#include <filesystem>
#include <string>

namespace ember
{
// Editor-facing operations for changing persisted import metadata.
// Runtime asset references are not modified; callers rescan their AssetManager
// before subsequent loads should use the updated settings.
class AssetImportService
{
public:
    explicit AssetImportService(std::filesystem::path assetRoot = "assets");

    [[nodiscard]] bool Scan();
    [[nodiscard]] const AssetMetadata* FindAsset(const std::string& assetId) const noexcept;
    [[nodiscard]] const AssetRegistry& GetRegistry() const noexcept;

    [[nodiscard]] bool UpdateTextureSettings(
        const std::string& assetId,
        const TextureImportSettings& settings,
        std::string* error = nullptr);

private:
    AssetRegistry m_registry;
};
} // namespace ember
