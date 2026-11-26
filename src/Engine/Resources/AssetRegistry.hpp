#pragma once

#include "Engine/Resources/AssetDirectory.hpp"
#include "Engine/Resources/AssetMetadata.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ember
{
enum class AssetRegistryDiagnosticType
{
    ScanError,
    InvalidMetadata,
    MissingSource,
    DuplicateAssetId
};

struct AssetRegistryDiagnostic
{
    AssetRegistryDiagnosticType type = AssetRegistryDiagnosticType::ScanError;
    std::filesystem::path path;
    std::string assetId;
    std::string message;
};

// Scans .emberasset sidecars and indexes valid metadata by stable asset ID.
// Assets with missing source files remain discoverable and carry a diagnostic;
// IDs shared by multiple sidecars are excluded to avoid ambiguous resolution.
class AssetRegistry
{
public:
    explicit AssetRegistry(std::filesystem::path assetRoot = "assets");

    // Returns false only when the directory scan itself cannot be completed.
    // Individual malformed sidecars and missing sources are reported as diagnostics.
    [[nodiscard]] bool Scan();

    [[nodiscard]] const AssetMetadata* Find(const std::string& assetId) const noexcept;
    [[nodiscard]] std::optional<std::filesystem::path> FindMetadataPath(
        const std::string& assetId) const;
    [[nodiscard]] const std::unordered_map<std::string, AssetMetadata>& GetAssets() const noexcept;
    [[nodiscard]] const std::vector<AssetRegistryDiagnostic>& GetDiagnostics() const noexcept;
    [[nodiscard]] std::size_t GetAssetCount() const noexcept;
    [[nodiscard]] const AssetDirectory& GetDirectory() const noexcept;

private:
    AssetDirectory m_directory;
    std::unordered_map<std::string, AssetMetadata> m_assets;
    std::unordered_map<std::string, std::filesystem::path> m_metadataPaths;
    std::vector<AssetRegistryDiagnostic> m_diagnostics;
};
} // namespace ember
