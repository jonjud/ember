#include "Editor/EditorApplication.hpp"
#include "Engine/Core/FileSystem.hpp"

#include <filesystem>

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
{
    const std::filesystem::path projectFile = argc > 1
        ? std::filesystem::path(argv[1]) : std::filesystem::path{};
#else
int main(int argc, char** argv)
{
    const std::filesystem::path projectFile = argc > 1
        ? ember::PathFromUtf8(argv[1]) : std::filesystem::path{};
#endif
    ember::EditorApplication application;
    if (!application.Initialize(projectFile))
        return 1;

    application.Run();
    return 0;
}
