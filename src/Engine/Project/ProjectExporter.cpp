#include "Engine/Project/ProjectExporter.hpp"

#include "Engine/Core/FileSystem.hpp"
#include "Engine/Project/ProjectConfigurationStore.hpp"
#include "Engine/Resources/AssetRegistry.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>
#include <vector>

namespace ember
{
namespace
{
using Json = nlohmann::json;

bool Fail(std::string* error, const std::string& message)
{
    if (error) *error = message;
    return false;
}

bool IsWithin(const std::filesystem::path& child, const std::filesystem::path& parent)
{
    const auto relative = child.lexically_relative(parent);
    if (relative.empty()) return child == parent;
    if (relative.is_absolute()) return false;
    for (const auto& part : relative)
        if (part == "..") return false;
    return true;
}

bool CopyRegularFile(const std::filesystem::path& source,
    const std::filesystem::path& destination, std::size_t& count, std::string* error)
{
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(source, ec);
    if (ec || std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status))
        return Fail(error, "Export encountered a non-regular file or symbolic link: " + PathToUtf8(source));
    std::filesystem::create_directories(destination.parent_path(), ec);
    if (ec) return Fail(error, "Could not create export subdirectory: " + ec.message());
    std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) return Fail(error, "Could not copy project file: " + PathToUtf8(source) + " (" + ec.message() + ")");
    ++count;
    return true;
}

bool CopyRuntimeLibraries(const std::filesystem::path& runtimeDirectory,
    const std::filesystem::path& destinationDirectory, std::size_t& count,
    std::vector<std::string>& copiedLibraries, std::string* error)
{
    std::error_code ec;
    std::filesystem::directory_iterator it(runtimeDirectory, ec), end;
    if (ec) return Fail(error, "Could not inspect runtime directory: " + ec.message());
    for (; it != end; it.increment(ec))
    {
        if (ec) return Fail(error, "Could not continue runtime dependency scan: " + ec.message());
        const auto source = it->path();
        auto extension = PathToUtf8(source.extension());
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (extension != ".dll") continue;
        if (!CopyRegularFile(source, destinationDirectory / source.filename(), count, error))
            return false;
        copiedLibraries.push_back(PathToUtf8(source.filename()));
    }
    std::sort(copiedLibraries.begin(), copiedLibraries.end());
    return true;
}

bool CopyDirectoryTree(const std::filesystem::path& sourceRoot,
    const std::filesystem::path& destinationRoot, std::size_t& count, std::string* error)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(sourceRoot, ec) || ec)
        return Fail(error, "Configured asset directory does not exist.");
    std::filesystem::create_directories(destinationRoot, ec);
    if (ec) return Fail(error, "Could not create exported asset directory: " + ec.message());

    std::filesystem::recursive_directory_iterator it(
        sourceRoot, std::filesystem::directory_options::none, ec), end;
    if (ec) return Fail(error, "Could not scan asset directory: " + ec.message());
    for (; it != end; it.increment(ec))
    {
        if (ec) return Fail(error, "Could not continue asset scan: " + ec.message());
        const auto source = it->path();
        const auto status = it->symlink_status(ec);
        if (ec) return Fail(error, "Could not inspect asset: " + PathToUtf8(source));
        if (std::filesystem::is_symlink(status))
        {
            it.disable_recursion_pending();
            return Fail(error, "Export does not allow symbolic links: " + PathToUtf8(source));
        }
        const auto relative = source.lexically_relative(sourceRoot);
        const auto destination = destinationRoot / relative;
        if (std::filesystem::is_directory(status))
        {
            std::filesystem::create_directories(destination, ec);
            if (ec) return Fail(error, "Could not create asset subdirectory: " + ec.message());
        }
        else if (std::filesystem::is_regular_file(status))
        {
            if (!CopyRegularFile(source, destination, count, error)) return false;
        }
        else
        {
            return Fail(error, "Export encountered an unsupported filesystem entry: " + PathToUtf8(source));
        }
    }
    return true;
}
}

bool ProjectExporter::ExportWindows(
    const std::filesystem::path& projectFile,
    const std::filesystem::path& runtimeExecutable,
    const std::filesystem::path& outputDirectory,
    ProjectExportResult* result,
    std::string* error)
{
    if (projectFile.empty() || runtimeExecutable.empty() || outputDirectory.empty())
        return Fail(error, "Project, runtime executable and output directory are required.");

    std::error_code ec;
    const auto project = std::filesystem::weakly_canonical(projectFile, ec);
    if (ec || !std::filesystem::is_regular_file(project, ec) || ec)
        return Fail(error, "Project file does not exist.");
    const auto runtime = std::filesystem::weakly_canonical(runtimeExecutable, ec);
    if (ec || !std::filesystem::is_regular_file(runtime, ec) || ec)
        return Fail(error, "Runtime executable does not exist.");

    const auto configuration = ProjectConfigurationStore::Load(project, error);
    if (!configuration) return false;
    const auto projectRoot = project.parent_path();
    const auto assetRoot = ProjectConfigurationStore::ResolveAssetDirectory(project, *configuration);
    if (!assetRoot || !IsWithin(std::filesystem::absolute(*assetRoot).lexically_normal(),
            std::filesystem::absolute(projectRoot).lexically_normal()))
        return Fail(error, "Asset directory must remain inside the project directory.");

    if (configuration->startupScene.empty())
        return Fail(error, "A startup scene is required for a Windows game export.");
    std::filesystem::path startupScene;
    const auto resolvedScene = ProjectConfigurationStore::ResolveStartupScene(project, *configuration);
    if (!resolvedScene || !IsWithin(std::filesystem::absolute(*resolvedScene).lexically_normal(),
            std::filesystem::absolute(projectRoot).lexically_normal()))
        return Fail(error, "Startup scene must remain inside the project directory.");
    startupScene = *resolvedScene;
    if (!std::filesystem::is_regular_file(startupScene, ec) || ec)
        return Fail(error, "Configured startup scene does not exist.");

    AssetRegistry registry(*assetRoot);
    if (!registry.Scan()) return Fail(error, "Could not scan project assets.");
    if (!registry.GetDiagnostics().empty())
        return Fail(error, "Asset registry contains errors; fix missing or invalid asset metadata before exporting.");

    const auto outputCandidate = std::filesystem::absolute(outputDirectory).lexically_normal();
    const auto output = std::filesystem::weakly_canonical(outputCandidate, ec);
    if (ec)
        return Fail(error, "Could not resolve export destination: " + ec.message());
    const auto canonicalProjectRoot = std::filesystem::weakly_canonical(projectRoot, ec);
    if (ec)
        return Fail(error, "Could not resolve source project directory: " + ec.message());
    if (IsWithin(output, canonicalProjectRoot))
        return Fail(error, "Export destination must be outside the source project directory.");
    if (std::filesystem::exists(output, ec))
    {
        if (ec || !std::filesystem::is_directory(output, ec) || ec ||
            !std::filesystem::is_empty(output, ec) || ec)
            return Fail(error, "Export destination must not exist or must be an empty directory.");
    }
    else
    {
        std::filesystem::create_directories(output, ec);
        if (ec) return Fail(error, "Could not create export destination: " + ec.message());
    }

    std::size_t copied = 0;
    std::vector<std::string> runtimeLibraries;
    const auto runtimeDestination = output / "ember_runtime.exe";
    if (!CopyRegularFile(runtime, runtimeDestination, copied, error)) return false;
    if (!CopyRuntimeLibraries(runtime.parent_path(), output, copied, runtimeLibraries, error)) return false;

    const auto projectDestination = output / project.filename();
    if (!CopyRegularFile(project, projectDestination, copied, error)) return false;

    const auto assetRelative = assetRoot->lexically_relative(projectRoot);
    if (assetRelative.empty() || assetRelative.is_absolute())
        return Fail(error, "Asset directory path is not project-relative.");
    if (!CopyDirectoryTree(*assetRoot, output / assetRelative, copied, error)) return false;

    if (!IsWithin(startupScene, *assetRoot))
    {
        const auto sceneRelative = startupScene.lexically_relative(projectRoot);
        if (sceneRelative.empty() || sceneRelative.is_absolute() ||
            !CopyRegularFile(startupScene, output / sceneRelative, copied, error))
            return Fail(error, "Could not include the configured startup scene in the export.");
    }

    const Json manifest{
        {"format_version", 1},
        {"platform", "windows-x64"},
        {"runtime", "ember_runtime.exe"},
        {"runtime_dependencies", runtimeLibraries},
        {"project", PathToUtf8(project.filename())},
        {"project_name", configuration->name},
        {"startup_scene", PathToUtf8(startupScene.lexically_relative(projectRoot))},
        {"copied_files", copied}
    };
    const auto manifestText = manifest.dump(4) + "\n";
    if (!WriteFileAtomically(output / "ember-export.json", manifestText, false, error))
        return false;

    if (result)
    {
        result->outputDirectory = output;
        result->runtimeExecutable = runtimeDestination;
        result->projectFile = projectDestination;
        result->copiedFiles = copied;
    }
    if (error) error->clear();
    return true;
}
} // namespace ember
