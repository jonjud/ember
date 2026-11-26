#include "Engine/Project/ProjectConfigurationStore.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <system_error>

namespace ember
{
namespace
{
using Json = nlohmann::json;

bool IsSafeRelativePath(const std::filesystem::path& path, bool allowEmpty)
{
    if (path.empty())
    {
        return allowEmpty;
    }
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory())
    {
        return false;
    }

    for (const auto& part : path)
    {
        if (part == "..")
        {
            return false;
        }
    }
    return true;
}

bool Fail(std::string* error, const std::string& message)
{
    if (error != nullptr)
    {
        *error = message;
    }
    return false;
}

std::optional<std::filesystem::path> ResolveRelative(
    const std::filesystem::path& projectFilePath,
    const std::filesystem::path& relativePath,
    bool allowEmpty)
{
    if (projectFilePath.empty() || !IsSafeRelativePath(relativePath, allowEmpty) ||
        relativePath.empty())
    {
        return std::nullopt;
    }

    const auto projectDirectory =
        std::filesystem::absolute(projectFilePath).parent_path().lexically_normal();
    return (projectDirectory / relativePath).lexically_normal();
}

bool Validate(const ProjectConfiguration& configuration, std::string* error)
{
    if (configuration.schemaVersion != ProjectConfiguration::CurrentSchemaVersion)
    {
        return Fail(error, "Unsupported project configuration schema version.");
    }
    if (configuration.name.empty())
    {
        return Fail(error, "Project name must not be empty.");
    }
    if (!IsSafeRelativePath(configuration.assetDirectory, false))
    {
        return Fail(error, "Asset directory must be a safe project-relative path.");
    }
    if (!IsSafeRelativePath(configuration.startupScene, true))
    {
        return Fail(error, "Startup scene must be a safe project-relative path.");
    }
    for (const auto& [action, keys] : configuration.inputBindings)
    {
        if (action.empty())
        {
            return Fail(error, "Input action names must not be empty.");
        }
        for (const std::int32_t key : keys)
        {
            if (key <= 0 || key >= 512)
            {
                return Fail(error, "Input bindings contain an invalid scancode.");
            }
        }
    }
    return true;
}
}

bool ProjectConfigurationStore::Save(
    const std::filesystem::path& projectFilePath,
    const ProjectConfiguration& configuration,
    std::string* error)
{
    if (projectFilePath.empty())
    {
        return Fail(error, "Project file path is empty.");
    }
    if (!Validate(configuration, error))
    {
        return false;
    }

    try
    {
        const Json document{
            {"schema_version", configuration.schemaVersion},
            {"name", configuration.name},
            {"asset_directory", configuration.assetDirectory.generic_string()},
            {"startup_scene", configuration.startupScene.generic_string()},
            {"input_bindings", configuration.inputBindings}
        };

        const auto parent = projectFilePath.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent);
        }

        std::ofstream output(projectFilePath, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            return Fail(error, "Could not open project file for writing.");
        }

        output << document.dump(4) << '\n';
        if (!output)
        {
            return Fail(error, "Could not write project configuration.");
        }
    }
    catch (const std::exception& exception)
    {
        return Fail(error, exception.what());
    }

    if (error != nullptr)
    {
        error->clear();
    }
    return true;
}

std::optional<ProjectConfiguration> ProjectConfigurationStore::Load(
    const std::filesystem::path& projectFilePath,
    std::string* error)
{
    try
    {
        std::ifstream input(projectFilePath, std::ios::binary);
        if (!input)
        {
            if (error != nullptr)
            {
                *error = "Could not open project file.";
            }
            return std::nullopt;
        }

        const Json document = Json::parse(input);
        ProjectConfiguration configuration;
        const std::uint32_t storedSchemaVersion =
            document.at("schema_version").get<std::uint32_t>();
        if (storedSchemaVersion != 1 &&
            storedSchemaVersion != ProjectConfiguration::CurrentSchemaVersion)
        {
            if (error != nullptr)
            {
                *error = "Unsupported project configuration schema version.";
            }
            return std::nullopt;
        }
        configuration.schemaVersion = ProjectConfiguration::CurrentSchemaVersion;
        configuration.name = document.at("name").get<std::string>();
        configuration.assetDirectory =
            document.at("asset_directory").get<std::string>();
        configuration.startupScene =
            document.at("startup_scene").get<std::string>();
        if (storedSchemaVersion >= 2 && document.contains("input_bindings"))
        {
            configuration.inputBindings =
                document.at("input_bindings").get<std::map<std::string, std::vector<std::int32_t>>>();
            configuration.inputBindingsConfigured = true;
        }

        if (!Validate(configuration, error))
        {
            return std::nullopt;
        }

        if (error != nullptr)
        {
            error->clear();
        }
        return configuration;
    }
    catch (const std::exception& exception)
    {
        if (error != nullptr)
        {
            *error = exception.what();
        }
        return std::nullopt;
    }
}

std::optional<std::filesystem::path> ProjectConfigurationStore::ResolveAssetDirectory(
    const std::filesystem::path& projectFilePath,
    const ProjectConfiguration& configuration)
{
    return ResolveRelative(projectFilePath, configuration.assetDirectory, false);
}

std::optional<std::filesystem::path> ProjectConfigurationStore::ResolveStartupScene(
    const std::filesystem::path& projectFilePath,
    const ProjectConfiguration& configuration)
{
    return ResolveRelative(projectFilePath, configuration.startupScene, true);
}
} // namespace ember
