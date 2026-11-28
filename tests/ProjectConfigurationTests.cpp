#include "Engine/Project/ProjectConfigurationStore.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int g_failures = 0;

void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << description << '\n';
        ++g_failures;
    }
    else
    {
        std::cout << "[PASS] " << description << '\n';
    }
}
}

int main()
{
    const auto root =
        std::filesystem::temp_directory_path() / "ember_project_configuration_tests";
    std::error_code filesystemError;
    std::filesystem::remove_all(root, filesystemError);
    std::filesystem::create_directories(root, filesystemError);

    const auto projectFile = root / "Sample.emberproject";
    ember::ProjectConfiguration configuration;
    configuration.name = "Sample Game";
    configuration.assetDirectory = "Content/Assets";
    configuration.startupScene = "Scenes/Main.emberscene";
    configuration.inputBindings["MoveLeft"] = {4, 80};
    configuration.inputBindings["Jump"] = {44};

    std::string error;
    Check(ember::ProjectConfigurationStore::Save(projectFile, configuration, &error),
          "Project configuration can be saved");
    Check(error.empty(), "Successful save clears the error message");

    ember::ProjectConfiguration updatedConfiguration = configuration;
    updatedConfiguration.name = "Updated Game";
    Check(ember::ProjectConfigurationStore::Save(
        projectFile, updatedConfiguration, &error),
        "Updating a project configuration succeeds");
    std::filesystem::path backupFile = projectFile;
    backupFile += ".bak";
    const auto backup = ember::ProjectConfigurationStore::Load(backupFile, &error);
    Check(backup.has_value() && backup->name == "Sample Game",
        "Project save preserves the previous configuration in a .bak file");
    Check(ember::ProjectConfigurationStore::Save(projectFile, configuration, &error),
        "Original project configuration can be restored after backup verification");

    const auto loaded = ember::ProjectConfigurationStore::Load(projectFile, &error);
    Check(loaded.has_value(), "Project configuration can be loaded");
    if (loaded)
    {
        Check(loaded->name == "Sample Game", "Project name survives round-trip");
        Check(loaded->assetDirectory == std::filesystem::path("Content/Assets"),
              "Asset directory survives round-trip");
        Check(loaded->startupScene == std::filesystem::path("Scenes/Main.emberscene"),
              "Startup scene survives round-trip");
        Check(loaded->inputBindings.at("MoveLeft") == std::vector<std::int32_t>{4, 80},
              "Multiple input bindings survive round-trip");
        Check(loaded->inputBindings.at("Jump") == std::vector<std::int32_t>{44},
              "Custom action bindings survive round-trip");

        const auto resolvedAssets =
            ember::ProjectConfigurationStore::ResolveAssetDirectory(projectFile, *loaded);
        Check(resolvedAssets.has_value() &&
              *resolvedAssets == (root / "Content" / "Assets").lexically_normal(),
              "Asset directory resolves relative to the project file");

        const auto resolvedScene =
            ember::ProjectConfigurationStore::ResolveStartupScene(projectFile, *loaded);
        Check(resolvedScene.has_value() &&
              *resolvedScene == (root / "Scenes" / "Main.emberscene").lexically_normal(),
              "Startup scene resolves relative to the project file");
    }

    configuration.inputBindings.clear();
    configuration.inputBindingsConfigured = true;
    Check(ember::ProjectConfigurationStore::Save(projectFile, configuration, &error),
          "An intentionally empty input map can be saved");
    const auto emptyBindings = ember::ProjectConfigurationStore::Load(projectFile, &error);
    Check(emptyBindings.has_value() && emptyBindings->inputBindingsConfigured &&
          emptyBindings->inputBindings.empty(),
          "An intentionally empty input map remains empty after loading");

    configuration.startupScene.clear();
    Check(ember::ProjectConfigurationStore::Save(projectFile, configuration, &error),
          "A project may omit its startup scene");
    const auto noStartupScene = ember::ProjectConfigurationStore::Load(projectFile, &error);
    Check(noStartupScene.has_value() && noStartupScene->startupScene.empty(),
          "An omitted startup scene remains empty");
    if (noStartupScene)
    {
        Check(!ember::ProjectConfigurationStore::ResolveStartupScene(
                  projectFile, *noStartupScene).has_value(),
              "An omitted startup scene does not resolve to a path");
    }

    configuration.assetDirectory = "../outside";
    Check(!ember::ProjectConfigurationStore::Save(projectFile, configuration, &error),
          "Parent traversal in asset directory is rejected");

    configuration.assetDirectory = "assets";
    configuration.startupScene = "../outside.emberscene";
    Check(!ember::ProjectConfigurationStore::Save(projectFile, configuration, &error),
          "Parent traversal in startup scene is rejected");

    configuration.startupScene.clear();
    configuration.name.clear();
    Check(!ember::ProjectConfigurationStore::Save(projectFile, configuration, &error),
          "Empty project names are rejected");

    configuration.name = "Sample Game";
    configuration.inputBindings["Invalid"] = {0};
    Check(!ember::ProjectConfigurationStore::Save(projectFile, configuration, &error),
          "Unknown scancode values are rejected");

    std::ofstream(projectFile, std::ios::trunc)
        << R"({"schema_version":1,"name":"Legacy","asset_directory":"assets","startup_scene":""})";
    const auto legacy = ember::ProjectConfigurationStore::Load(projectFile, &error);
    Check(legacy.has_value() && legacy->inputBindings.empty(),
          "Version 1 project files migrate with empty input settings");

    std::ofstream(projectFile, std::ios::trunc)
        << R"({"schema_version":99,"name":"Future","asset_directory":"assets","startup_scene":""})";
    Check(!ember::ProjectConfigurationStore::Load(projectFile, &error),
          "Unsupported project schema versions are rejected");

    std::filesystem::remove_all(root, filesystemError);

    if (g_failures != 0)
    {
        std::cerr << g_failures << " project configuration test(s) failed.\n";
        return 1;
    }

    std::cout << "All project configuration tests passed.\n";
    return 0;
}
