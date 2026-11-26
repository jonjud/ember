#pragma once

#include "Engine/Resources/AssetMetadata.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace ember
{
class AssetMetadataStore
{
public:
    // Writes a JSON sidecar file. The parent directory is created when needed.
    [[nodiscard]] static bool Save(
        const std::filesystem::path& metadataPath,
        const AssetMetadata& metadata,
        std::string* error = nullptr);

    [[nodiscard]] static std::optional<AssetMetadata> Load(
        const std::filesystem::path& metadataPath,
        std::string* error = nullptr);
};
} // namespace ember
