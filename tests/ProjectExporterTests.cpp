#include "Engine/Project/ProjectExporter.hpp"
#include "Engine/Project/ProjectConfigurationStore.hpp"
#include "Engine/Resources/AssetMetadataStore.hpp"
#include "Engine/Core/FileSystem.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
int failures = 0;
void Check(bool value, const char* message)
{
    if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
void Write(const std::filesystem::path& path, const std::string& value)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << value;
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "ember_project_exporter_tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    const auto projectRoot = root / "source";
    const auto output = root / "export";
    const auto projectFile = projectRoot / "demo.emberproject";
    const auto runtime = root / "ember_runtime.exe";
    Write(runtime, "runtime-binary");
    Write(projectRoot / "assets" / "textures" / "hero.png", "fake-image");
    Write(projectRoot / "scenes" / "main.ember", "scene-data");

    ember::ProjectConfiguration config;
    config.name = "Demo Game";
    config.assetDirectory = "assets";
    config.startupScene = "scenes/main.ember";
    std::string error;
    Check(ember::ProjectConfigurationStore::Save(projectFile, config, &error), "save project config");

    ember::AssetMetadata texture;
    texture.assetId = "texture.hero";
    texture.type = ember::AssetType::Texture;
    texture.source = "textures/hero.png";
    Check(ember::AssetMetadataStore::Save(
        projectRoot / "assets" / "textures" / "hero.png.emberasset", texture, &error),
        "save asset metadata");

    ember::ProjectExportResult result;
    Check(ember::ProjectExporter::ExportWindows(projectFile, runtime, output, &result, &error),
        "export valid project");
    Check(std::filesystem::exists(output / "ember_runtime.exe"), "runtime copied");
    Check(std::filesystem::exists(output / "demo.emberproject"), "project file copied");
    Check(std::filesystem::exists(output / "assets" / "textures" / "hero.png"), "asset source copied");
    Check(std::filesystem::exists(output / "assets" / "textures" / "hero.png.emberasset"), "asset metadata copied");
    Check(std::filesystem::exists(output / "scenes" / "main.ember"), "external startup scene copied");
    Check(std::filesystem::exists(output / "ember-export.json"), "manifest written");
    Check(result.copiedFiles == 5, "copied file count excludes manifest");

    const auto unsafeOutput = projectRoot / "inside";
    Check(!ember::ProjectExporter::ExportWindows(projectFile, runtime, unsafeOutput, nullptr, &error),
        "reject export inside source project");
    const auto nonEmpty = root / "non-empty";
    Write(nonEmpty / "keep.txt", "keep");
    Check(!ember::ProjectExporter::ExportWindows(projectFile, runtime, nonEmpty, nullptr, &error),
        "reject non-empty destination");

    std::filesystem::remove_all(root, ec);
    return failures == 0 ? 0 : 1;
}
