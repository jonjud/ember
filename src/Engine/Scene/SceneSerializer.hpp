#pragma once

#include "Engine/Scene/Scene.hpp"

#include <filesystem>
#include <string>

namespace ember
{
class SceneSerializer
{
public:
    [[nodiscard]] static bool Save(
        const Scene& scene,
        const std::filesystem::path& filePath,
        std::string* error = nullptr);

    // Loads into a temporary scene and replaces the destination only on success.
    [[nodiscard]] static bool Load(
        Scene& scene,
        const std::filesystem::path& filePath,
        std::string* error = nullptr);
};
} // namespace ember
