#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace ember
{
enum class AssetType
{
    Texture,
    Audio,
    Scene,
    Script
};

enum class TextureFilter
{
    Nearest,
    Linear
};

struct TextureImportSettings
{
    TextureFilter filter = TextureFilter::Nearest;
    bool premultiplyAlpha = false;
};

// Stable, project-relative description of an imported asset.
// Runtime handles are deliberately not stored in metadata.
struct AssetMetadata
{
    static constexpr std::uint32_t CurrentSchemaVersion = 1;

    std::uint32_t schemaVersion = CurrentSchemaVersion;
    std::string assetId;
    AssetType type = AssetType::Texture;
    std::filesystem::path source;
    TextureImportSettings texture;
    // Empty for language-neutral/unassigned Script assets; e.g. .lua imports use "lua".
    std::string scriptLanguage;
};
} // namespace ember
