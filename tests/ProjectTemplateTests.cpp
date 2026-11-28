#include "Engine/Project/ProjectTemplateService.hpp"
#include "Engine/Project/ProjectConfigurationStore.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/SceneSerializer.hpp"
#include "Engine/Resources/AssetRegistry.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int failures = 0;
void Check(bool value, const char* message)
{
    if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "ember_project_template_tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    const auto& templates = ember::ProjectTemplateService::GetTemplates();
    Check(templates.size() == 2, "two built-in 2D templates are available");

    ember::ProjectTemplateResult blank;
    std::string error;
    Check(ember::ProjectTemplateService::CreateProject(
        root / "BlankGame", "Blank Game", ember::ProjectTemplateId::Blank2D, &blank, &error),
        "Blank 2D project is created");
    Check(std::filesystem::is_directory(blank.projectDirectory / "assets"),
        "Blank project contains an asset directory");
    const auto blankConfig = ember::ProjectConfigurationStore::Load(blank.projectFile, &error);
    Check(blankConfig.has_value() && blankConfig->name == "Blank Game",
        "Blank project configuration is valid");
    Check(blankConfig.has_value() && blankConfig->startupScene == std::filesystem::path("scenes/main.emberscene"),
        "Blank project selects its startup scene");
    Check(blankConfig.has_value() &&
          blankConfig->inputBindings.at("MoveLeft") == std::vector<std::int32_t>{4, 80} &&
          blankConfig->inputBindings.at("MoveRight") == std::vector<std::int32_t>{7, 79},
        "Built-in movement actions and keyboard bindings are configured");
    ember::Scene blankScene;
    Check(ember::SceneSerializer::Load(blankScene, blank.startupScene, &error),
        "Blank startup scene can be loaded");
    Check(blankScene.IsEmpty(), "Blank startup scene has no entities");
    ember::AssetRegistry blankAssets(blank.projectDirectory / "assets");
    Check(blankAssets.Scan() && blankAssets.GetDiagnostics().empty(),
        "Blank project asset registry scans without diagnostics");

    ember::ProjectTemplateResult starter;
    Check(ember::ProjectTemplateService::CreateProject(
        root / "StarterGame", "Starter Game", ember::ProjectTemplateId::Starter2D, &starter, &error),
        "2D Starter project is created");
    const auto starterConfig = ember::ProjectConfigurationStore::Load(starter.projectFile, &error);
    Check(starterConfig.has_value(), "Starter project configuration is valid");
    ember::Scene starterScene;
    Check(ember::SceneSerializer::Load(starterScene, starter.startupScene, &error),
        "Starter startup scene can be loaded");
    Check(starterScene.GetEntityCount() == 1, "Starter scene contains one Player entity");
    const auto ids = starterScene.GetEntityIds();
    if (!ids.empty())
    {
        const auto* player = starterScene.FindEntity(ids.front());
        Check(player != nullptr && player->GetName() == "Player",
            "Starter scene Player entity is named correctly");
        Check(player != nullptr && player->GetMovementComponent().enabled,
            "Starter Player movement is enabled");
        Check(player != nullptr && player->GetRenderComponent().visible,
            "Starter Player is visible");
    }

    Check(!ember::ProjectTemplateService::CreateProject(
        root / "StarterGame", "Overwrite", ember::ProjectTemplateId::Blank2D, nullptr, &error),
        "Existing project destination is never overwritten");

    std::filesystem::remove_all(root, ec);
    return failures == 0 ? 0 : 1;
}
