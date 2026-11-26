#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <map>
#include <vector>

namespace ember
{
// Project-level settings stored in a .emberproject file.
// Paths are relative to the directory containing that project file.
struct ProjectConfiguration
{
    static constexpr std::uint32_t CurrentSchemaVersion = 2;

    std::uint32_t schemaVersion = CurrentSchemaVersion;
    std::string name = "Untitled Project";
    std::filesystem::path assetDirectory = "assets";
    std::filesystem::path startupScene;
    // SDL-independent physical key codes, serialized as integer scancodes.
    std::map<std::string, std::vector<std::int32_t>> inputBindings;
    bool inputBindingsConfigured = false;
};
} // namespace ember
