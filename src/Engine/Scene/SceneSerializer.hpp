#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/ComponentRegistry.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace ember
{
class SceneSerializer
{
public:
    [[nodiscard]] static bool Save(
        const Scene& scene,
        const std::filesystem::path& filePath,
        std::string* error = nullptr,
        const ComponentRegistry* componentRegistry = nullptr);

    [[nodiscard]] static bool SerializeToString(
        const Scene& scene,
        std::string& serialized,
        std::string* error = nullptr,
        const ComponentRegistry* componentRegistry = nullptr);

    // Deserializes into a temporary scene and replaces the destination only on success.
    [[nodiscard]] static bool LoadFromString(
        Scene& scene,
        std::string_view serialized,
        std::string* error = nullptr,
        const ComponentRegistry* componentRegistry = nullptr);

    // Loads into a temporary scene and replaces the destination only on success.
    [[nodiscard]] static bool Load(
        Scene& scene,
        const std::filesystem::path& filePath,
        std::string* error = nullptr,
        const ComponentRegistry* componentRegistry = nullptr);
};
} // namespace ember
