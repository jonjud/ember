#include "Engine/Resources/AssetImportService.hpp"

#include "Engine/Resources/AssetMetadataStore.hpp"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace ember
{
namespace
{
bool Fail(std::string* error, const std::string& message)
{
    if (error != nullptr) *error = message;
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

bool AssetImportService::ImportScript(
    const std::filesystem::path& relativeSource,
    std::string* error)
{
    const auto resolvedSource = m_registry.GetDirectory().Resolve(relativeSource);
    if (!resolvedSource)
        return Fail(error, "Script source must be a safe path inside the asset directory.");

    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(*resolvedSource, filesystemError) || filesystemError)
        return Fail(error, "Script source file does not exist or is not a regular file.");

    std::ifstream source(*resolvedSource, std::ios::binary);
    if (!source)
        return Fail(error, "Script source file could not be opened.");

    char sample[4096]{};
    source.read(sample, sizeof(sample));
    for (std::streamsize index = 0; index < source.gcount(); ++index)
        if (sample[index] == '\0')
            return Fail(error, "Script source appears to be a binary file.");

    const std::string normalizedSource = relativeSource.lexically_normal().generic_string();
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char character : normalizedSource)
    {
        hash ^= character;
        hash *= 1099511628211ULL;
    }

    std::ostringstream id;
    id << "script-" << std::hex << std::setfill('0') << std::setw(16) << hash;
    if (m_registry.Find(id.str()) != nullptr)
        return Fail(error, "A Script asset with this source path is already imported.");

    std::filesystem::path metadataPath = *resolvedSource;
    metadataPath += ".emberasset";
    if (std::filesystem::exists(metadataPath, filesystemError) || filesystemError)
        return Fail(error, "An asset metadata sidecar already exists for this source file.");

    AssetMetadata metadata;
    metadata.assetId = id.str();
    metadata.type = AssetType::Script;
    metadata.source = relativeSource.lexically_normal();
    if (!AssetMetadataStore::Save(metadataPath, metadata, error))
        return false;

    if (!m_registry.Scan())
    {
        std::error_code cleanupError;
        std::filesystem::remove(metadataPath, cleanupError);
        return Fail(error, "Script metadata was written, but the asset registry could not be refreshed.");
    }

    if (error != nullptr) error->clear();
    return true;
}

bool AssetImportService::UpdateTextureSettings(
    const std::string& assetId,
    const TextureImportSettings& settings,
    std::string* error)
{
    if (assetId.empty())
        return Fail(error, "Asset ID must not be empty.");

    const AssetMetadata* current = m_registry.Find(assetId);
    if (current == nullptr)
        return Fail(error, "Asset ID was not found or is ambiguous.");
    if (current->type != AssetType::Texture)
        return Fail(error, "Import settings can only be changed for texture assets.");

    const auto metadataPath = m_registry.FindMetadataPath(assetId);
    if (!metadataPath)
        return Fail(error, "Asset metadata sidecar could not be located.");

    AssetMetadata updated = *current;
    updated.texture = settings;
    if (!AssetMetadataStore::Save(*metadataPath, updated, error))
        return false;

    if (!m_registry.Scan())
        return Fail(error, "Settings were saved, but the asset registry could not be refreshed.");

    if (error != nullptr) error->clear();
    return true;
}
} // namespace ember
