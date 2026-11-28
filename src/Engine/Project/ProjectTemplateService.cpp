#include "Engine/Project/ProjectTemplateService.hpp"

#include "Engine/Project/ProjectConfigurationStore.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/SceneSerializer.hpp"

#include <fstream>
#include <system_error>

namespace ember
{
namespace
{
bool Fail(std::string* error, const std::string& message)
{
    if (error != nullptr) *error = message;
    return false;
}

bool IsKnownTemplate(ProjectTemplateId id)
{
    return id == ProjectTemplateId::Blank2D || id == ProjectTemplateId::Starter2D;
}
}

const std::vector<ProjectTemplateDescriptor>& ProjectTemplateService::GetTemplates()
{
    static const std::vector<ProjectTemplateDescriptor> templates{
        {ProjectTemplateId::Blank2D, "Blank 2D", "An empty 2D project with a ready-to-edit startup scene."},
        {ProjectTemplateId::Starter2D, "2D Starter", "A simple movable Player entity and a ready-to-run startup scene."}
    };
    return templates;
}

bool ProjectTemplateService::CreateProject(
    const std::filesystem::path& projectDirectory,
    const std::string& projectName,
    ProjectTemplateId templateId,
    ProjectTemplateResult* result,
    std::string* error)
{
    if (projectDirectory.empty() || projectName.empty())
        return Fail(error, "Project directory and name are required.");
    if (!IsKnownTemplate(templateId))
        return Fail(error, "Unknown project template.");

    std::error_code ec;
    const auto destination = std::filesystem::absolute(projectDirectory, ec).lexically_normal();
    if (ec || destination.filename().empty())
        return Fail(error, "Could not resolve the project destination.");
    if (std::filesystem::exists(destination, ec))
        return Fail(error, "Project destination already exists.");
    if (ec)
        return Fail(error, "Could not inspect the project destination: " + ec.message());

    const auto parent = destination.parent_path();
    std::filesystem::create_directories(parent, ec);
    if (ec)
        return Fail(error, "Could not create the parent directory: " + ec.message());

    auto staging = destination;
    staging += ".ember-tmp";
    if (std::filesystem::exists(staging, ec))
        return Fail(error, "A temporary project directory already exists.");
    if (ec || !std::filesystem::create_directory(staging, ec) || ec)
        return Fail(error, "Could not create the temporary project directory.");

    const auto cleanup = [&staging]()
    {
        std::error_code ignored;
        std::filesystem::remove_all(staging, ignored);
    };

    const auto assets = staging / "assets";
    const auto scenes = staging / "scenes";
    if (!std::filesystem::create_directories(assets, ec) || ec)
    {
        cleanup();
        return Fail(error, "Could not create the project asset directory.");
    }
    if (!std::filesystem::create_directories(scenes, ec) || ec)
    {
        cleanup();
        return Fail(error, "Could not create the project scenes directory.");
    }

    ProjectConfiguration configuration;
    configuration.name = projectName;
    configuration.assetDirectory = "assets";
    configuration.startupScene = "scenes/main.emberscene";
    configuration.inputBindings = {
        {"MoveLeft", {4, 80}},
        {"MoveRight", {7, 79}},
        {"MoveUp", {26, 82}},
        {"MoveDown", {22, 81}}
    };
    configuration.inputBindingsConfigured = true;
    const auto projectFile = staging / "project.emberproject";
    if (!ProjectConfigurationStore::Save(projectFile, configuration, error))
    {
        cleanup();
        return false;
    }

    Scene scene(templateId == ProjectTemplateId::Starter2D ? "2D Starter Scene" : "Main Scene");
    if (templateId == ProjectTemplateId::Starter2D)
    {
        Entity& player = scene.CreateEntity("Player");
        player.GetTransform().position = {320.0F, 240.0F};
        player.GetRenderComponent().size = {48.0F, 48.0F};
        player.GetRenderComponent().red = 80;
        player.GetRenderComponent().green = 170;
        player.GetRenderComponent().blue = 255;
        player.GetRenderComponent().visible = true;
        player.GetMovementComponent().enabled = true;
        player.GetMovementComponent().speed = 240.0F;
    }

    const auto startupScene = scenes / "main.emberscene";
    if (!SceneSerializer::Save(scene, startupScene, error))
    {
        cleanup();
        return false;
    }

    const auto readme = staging / "README.txt";
    {
        std::ofstream output(readme, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            cleanup();
            return Fail(error, "Could not write the project README.");
        }
        output << "EMBER project: " << projectName << "\n"
               << "Template: " << (templateId == ProjectTemplateId::Starter2D ? "2D Starter" : "Blank 2D") << "\n"
               << "Open project.emberproject with EMBER Editor.\n";
        output.flush();
        if (!output)
        {
            cleanup();
            return Fail(error, "Could not finish writing the project README.");
        }
    }

    std::filesystem::rename(staging, destination, ec);
    if (ec)
    {
        cleanup();
        return Fail(error, "Could not publish the new project: " + ec.message());
    }

    if (result != nullptr)
    {
        result->projectDirectory = destination;
        result->projectFile = destination / projectFile.filename();
        result->startupScene = destination / startupScene.lexically_relative(staging);
    }
    if (error != nullptr) error->clear();
    return true;
}
} // namespace ember
