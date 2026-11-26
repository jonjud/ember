#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace ember
{
// Project-level settings stored in a .emberproject file.
// Paths are relative to the directory containing that project file.
struct ProjectConfiguration
{
    static constexpr std::uint32_t CurrentSchemaVersion = 1;

    std::uint32_t schemaVersion = CurrentSchemaVersion;
    std::string name = "Untitled Project";
    std::filesystem::path assetDirectory = "assets";
    std::filesystem::path startupScene;
};
} // namespace ember
