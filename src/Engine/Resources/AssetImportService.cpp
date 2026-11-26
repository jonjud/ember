#include "Engine/Resources/AssetImportService.hpp"

#include "Engine/Resources/AssetMetadataStore.hpp"

#include <utility>

namespace ember
{
namespace
{
bool Fail(std::string* error, const std::string& message)
{
    if (error != nullptr)
    {
        *error = message;
    }
    return false;
}
}

AssetImportService::AssetImportService(std::filesystem::path assetRoot)
    : m_registry(std::move(assetRoot))
{
}

bool AssetImportService::Scan()
{
    return m_registry.Scan();
}

const AssetMetadata* AssetImportService::FindAsset(const std::string& assetId) const noexcept
{
    return m_registry.Find(assetId);
}

const AssetRegistry& AssetImportService::GetRegistry() const noexcept
{
    return m_registry;
}

bool AssetImportService::UpdateTextureSettings(
    const std::string& assetId,
    const TextureImportSettings& settings,
    std::string* error)
{
    if (assetId.empty())
    {
        return Fail(error, "Asset ID must not be empty.");
    }

    const AssetMetadata* current = m_registry.Find(assetId);
    if (current == nullptr)
    {
        return Fail(error, "Asset ID was not found or is ambiguous.");
    }
    if (current->type != AssetType::Texture)
    {
        return Fail(error, "Import settings can only be changed for texture assets.");
    }

    const auto metadataPath = m_registry.FindMetadataPath(assetId);
    if (!metadataPath)
    {
        return Fail(error, "Asset metadata sidecar could not be located.");
    }

    AssetMetadata updated = *current;
    updated.texture = settings;
    if (!AssetMetadataStore::Save(*metadataPath, updated, error))
    {
        return false;
    }

    if (!m_registry.Scan())
    {
        return Fail(error, "Settings were saved, but the asset registry could not be refreshed.");
    }

    if (error != nullptr)
    {
        error->clear();
    }
    return true;
}
} // namespace ember
