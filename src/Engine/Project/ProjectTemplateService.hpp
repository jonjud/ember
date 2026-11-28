#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ember
{
enum class ProjectTemplateId
{
    Blank2D,
    Starter2D
};

struct ProjectTemplateDescriptor
{
    ProjectTemplateId id = ProjectTemplateId::Blank2D;
    std::string name;
    std::string description;
};

struct ProjectTemplateResult
{
    std::filesystem::path projectDirectory;
    std::filesystem::path projectFile;
    std::filesystem::path startupScene;
};

class ProjectTemplateService
{
public:
    [[nodiscard]] static const std::vector<ProjectTemplateDescriptor>& GetTemplates();

    // Creates a new project in a destination that does not already exist.
    // The project is staged in a sibling temporary directory and published
    // with a directory rename only after all files have been written.
    [[nodiscard]] static bool CreateProject(
        const std::filesystem::path& projectDirectory,
        const std::string& projectName,
        ProjectTemplateId templateId,
        ProjectTemplateResult* result = nullptr,
        std::string* error = nullptr);
};
} // namespace ember
