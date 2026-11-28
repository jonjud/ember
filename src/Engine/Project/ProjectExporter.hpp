#pragma once

#include <filesystem>
#include <string>

namespace ember
{
struct ProjectExportResult
{
    std::filesystem::path outputDirectory;
    std::filesystem::path runtimeExecutable;
    std::filesystem::path projectFile;
    std::size_t copiedFiles = 0;
};

class ProjectExporter
{
public:
    // Creates a self-contained project folder around an already-built runtime executable.
    // The destination must not exist or must be empty. Symlinks are rejected.
    [[nodiscard]] static bool ExportWindows(
        const std::filesystem::path& projectFile,
        const std::filesystem::path& runtimeExecutable,
        const std::filesystem::path& outputDirectory,
        ProjectExportResult* result = nullptr,
        std::string* error = nullptr);
};
} // namespace ember
