#include "Engine/Resources/AssetMetadataStore.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <system_error>

namespace ember
{
namespace
{
using Json = nlohmann::json;

bool IsSafeRelativeSource(const std::filesystem::path& source)
{
    if (source.empty() || source.is_absolute() || source.has_root_name())
        return false;
    for (const auto& part : source)
        if (part == "..") return false;
    return true;
}

const char* ToString(AssetType type)
{
    switch (type)
    {
    case AssetType::Texture: return "texture";
    case AssetType::Audio: return "audio";
    case AssetType::Scene: return "scene";
    case AssetType::Script: return "script";
    }
    return nullptr;
}

const char* ToString(TextureFilter filter)
{
    switch (filter)
    {
    case TextureFilter::Nearest: return "nearest";
    case TextureFilter::Linear: return "linear";
    }
    return nullptr;
}

bool IsValidScriptLanguage(const std::string& language)
{
    if (language.empty()) return true;
    if (language.size() > 32U) return false;
    return std::all_of(language.begin(), language.end(), [](unsigned char character)
    {
        return (character >= 'a' && character <= 'z') ||
               (character >= '0' && character <= '9') ||
               character == '-' || character == '_';
    });
}

bool Fail(std::string* error, const std::string& message)
{
    if (error != nullptr) *error = message;
    return false;
}
}

bool AssetMetadataStore::Save(
    const std::filesystem::path& metadataPath,
    const AssetMetadata& metadata,
    std::string* error)
{
    if (metadataPath.empty())
        return Fail(error, "Metadata file path is empty.");
    if (metadata.schemaVersion != AssetMetadata::CurrentSchemaVersion)
        return Fail(error, "Unsupported asset metadata schema version.");
    if (metadata.assetId.empty())
        return Fail(error, "Asset ID must not be empty.");
    if (!IsSafeRelativeSource(metadata.source))
        return Fail(error, "Asset source must be a safe project-relative path.");

    const char* type = ToString(metadata.type);
    if (type == nullptr)
        return Fail(error, "Asset type is invalid.");
    if (!IsValidScriptLanguage(metadata.scriptLanguage))
        return Fail(error, "Script language must be a lowercase identifier of at most 32 characters.");
    if (metadata.type != AssetType::Script && !metadata.scriptLanguage.empty())
        return Fail(error, "Only Script assets may declare a script language.");

    Json document{
        {"schema_version", metadata.schemaVersion},
        {"asset_id", metadata.assetId},
        {"type", type},
        {"source", metadata.source.generic_string()}
    };

    if (metadata.type == AssetType::Script && !metadata.scriptLanguage.empty())
        document["script_language"] = metadata.scriptLanguage;

    if (metadata.type == AssetType::Texture)
    {
        const char* filter = ToString(metadata.texture.filter);
        if (filter == nullptr)
            return Fail(error, "Texture filter is invalid.");
        document["texture"] = {
            {"filter", filter},
            {"premultiply_alpha", metadata.texture.premultiplyAlpha}
        };
    }

    try
    {
        const auto parent = metadataPath.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        std::ofstream output(metadataPath, std::ios::binary | std::ios::trunc);
        if (!output) return Fail(error, "Could not open metadata file for writing.");
        output << document.dump(4) << '\n';
        if (!output) return Fail(error, "Could not write asset metadata.");
    }
    catch (const std::exception& exception)
    {
        return Fail(error, exception.what());
    }

    if (error != nullptr) error->clear();
    return true;
}

std::optional<AssetMetadata> AssetMetadataStore::Load(
    const std::filesystem::path& metadataPath,
    std::string* error)
{
    try
    {
        std::ifstream input(metadataPath, std::ios::binary);
        if (!input)
        {
            if (error != nullptr) *error = "Could not open metadata file.";
            return std::nullopt;
        }

        const Json document = Json::parse(input);
        AssetMetadata metadata;
        metadata.schemaVersion = document.at("schema_version").get<std::uint32_t>();
        if (metadata.schemaVersion != AssetMetadata::CurrentSchemaVersion)
        {
            if (error != nullptr) *error = "Unsupported asset metadata schema version.";
            return std::nullopt;
        }

        metadata.assetId = document.at("asset_id").get<std::string>();
        metadata.source = document.at("source").get<std::string>();
        const std::string type = document.at("type").get<std::string>();

        if (type == "texture")
        {
            metadata.type = AssetType::Texture;
            const Json& texture = document.at("texture");
            const std::string filter = texture.at("filter").get<std::string>();
            if (filter == "nearest") metadata.texture.filter = TextureFilter::Nearest;
            else if (filter == "linear") metadata.texture.filter = TextureFilter::Linear;
            else
            {
                if (error != nullptr) *error = "Unknown texture filter.";
                return std::nullopt;
            }
            metadata.texture.premultiplyAlpha = texture.at("premultiply_alpha").get<bool>();
        }
        else if (type == "audio") metadata.type = AssetType::Audio;
        else if (type == "scene") metadata.type = AssetType::Scene;
        else if (type == "script") metadata.type = AssetType::Script;
        else
        {
            if (error != nullptr) *error = "Unknown asset type.";
            return std::nullopt;
        }

        metadata.scriptLanguage = document.value("script_language", std::string{});
        if (!IsValidScriptLanguage(metadata.scriptLanguage))
        {
            if (error != nullptr) *error = "Script metadata contains an invalid language identifier.";
            return std::nullopt;
        }
        if (metadata.type != AssetType::Script && !metadata.scriptLanguage.empty())
        {
            if (error != nullptr) *error = "Only Script assets may declare a script language.";
            return std::nullopt;
        }

        if (metadata.assetId.empty() || !IsSafeRelativeSource(metadata.source))
        {
            if (error != nullptr) *error = "Asset metadata contains an invalid ID or source path.";
            return std::nullopt;
        }
        if (error != nullptr) error->clear();
        return metadata;
    }
    catch (const std::exception& exception)
    {
        if (error != nullptr) *error = exception.what();
        return std::nullopt;
    }
}
} // namespace ember
