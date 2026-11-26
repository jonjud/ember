#pragma once

#include "Engine/Project/ProjectConfiguration.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace ember
{
class ProjectConfigurationStore
{
public:
    [[nodiscard]] static bool Save(
        const std::filesystem::path& projectFilePath,
        const ProjectConfiguration& configuration,
        std::string* error = nullptr);

    [[nodiscard]] static std::optional<ProjectConfiguration> Load(
        const std::filesystem::path& projectFilePath,
        std::string* error = nullptr);

    // Resolve configured paths relative to the .emberproject file's directory.
    // These are lexical checks and do not prevent symlink escapes.
    [[nodiscard]] static std::optional<std::filesystem::path> ResolveAssetDirectory(
        const std::filesystem::path& projectFilePath,
        const ProjectConfiguration& configuration);

    [[nodiscard]] static std::optional<std::filesystem::path> ResolveStartupScene(
        const std::filesystem::path& projectFilePath,
        const ProjectConfiguration& configuration);
};
} // namespace ember
