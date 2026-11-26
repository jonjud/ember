#include "Engine/Resources/AssetRegistry.hpp"

#include "Engine/Resources/AssetMetadataStore.hpp"

#include <algorithm>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace ember
{
AssetRegistry::AssetRegistry(std::filesystem::path assetRoot)
    : m_directory(std::move(assetRoot))
{
}

bool AssetRegistry::Scan()
{
    m_assets.clear();
    m_diagnostics.clear();

    const auto& root = m_directory.GetRoot();
    std::error_code error;
    if (!std::filesystem::is_directory(root, error) || error)
    {
        m_diagnostics.push_back({
            AssetRegistryDiagnosticType::ScanError,
            root,
            {},
            error ? error.message() : "Asset root directory does not exist."
        });
        return false;
    }

    std::vector<std::filesystem::path> sidecars;
    std::filesystem::recursive_directory_iterator iterator(
        root, std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::recursive_directory_iterator end;

    if (error)
    {
        m_diagnostics.push_back({
            AssetRegistryDiagnosticType::ScanError, root, {}, error.message()
        });
        return false;
    }

    bool scanComplete = true;
    while (iterator != end)
    {
        const auto path = iterator->path();
        std::error_code statusError;
        if (iterator->is_regular_file(statusError) &&
            path.extension() == ".emberasset")
        {
            sidecars.push_back(path);
        }
        else if (statusError)
        {
            m_diagnostics.push_back({
                AssetRegistryDiagnosticType::ScanError,
                path,
                {},
                statusError.message()
            });
        }

        iterator.increment(error);
        if (error)
        {
            m_diagnostics.push_back({
                AssetRegistryDiagnosticType::ScanError,
                path,
                {},
                error.message()
            });
            scanComplete = false;
            break;
        }
    }

    std::sort(sidecars.begin(), sidecars.end());

    std::unordered_set<std::string> duplicateIds;
    for (const auto& sidecar : sidecars)
    {
        std::string loadError;
        const auto metadata = AssetMetadataStore::Load(sidecar, &loadError);
        if (!metadata)
        {
            m_diagnostics.push_back({
                AssetRegistryDiagnosticType::InvalidMetadata,
                sidecar,
                {},
                std::move(loadError)
            });
            continue;
        }

        const auto resolvedSource = m_directory.Resolve(metadata->source);
        std::error_code sourceError;
        if (!resolvedSource ||
            !std::filesystem::is_regular_file(*resolvedSource, sourceError) ||
            sourceError)
        {
            m_diagnostics.push_back({
                AssetRegistryDiagnosticType::MissingSource,
                sidecar,
                metadata->assetId,
                sourceError ? sourceError.message() : "Asset source file is missing."
            });
        }

        const std::string& id = metadata->assetId;
        if (duplicateIds.contains(id))
        {
            continue;
        }

        const auto [entry, inserted] = m_assets.emplace(id, *metadata);
        if (!inserted)
        {
            m_assets.erase(entry);
            duplicateIds.insert(id);
            m_diagnostics.push_back({
                AssetRegistryDiagnosticType::DuplicateAssetId,
                sidecar,
                id,
                "Multiple metadata sidecars use the same asset ID; the ID was excluded."
            });
        }
    }

    return scanComplete;
}

const AssetMetadata* AssetRegistry::Find(const std::string& assetId) const noexcept
{
    const auto found = m_assets.find(assetId);
    return found == m_assets.end() ? nullptr : &found->second;
}

const std::unordered_map<std::string, AssetMetadata>& AssetRegistry::GetAssets() const noexcept
{
    return m_assets;
}

const std::vector<AssetRegistryDiagnostic>& AssetRegistry::GetDiagnostics() const noexcept
{
    return m_diagnostics;
}

std::size_t AssetRegistry::GetAssetCount() const noexcept
{
    return m_assets.size();
}

const AssetDirectory& AssetRegistry::GetDirectory() const noexcept
{
    return m_directory;
}
} // namespace ember
