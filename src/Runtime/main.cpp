#include "Runtime/RuntimeApplication.hpp"
#include "Engine/Core/FileSystem.hpp"

#include <SDL3/SDL.h>
#include <filesystem>
#include <iostream>
#include <vector>

namespace
{
std::filesystem::path FindProjectBesideExecutable()
{
    char* basePath = SDL_GetBasePath();
    if (basePath == nullptr) return {};
    const std::filesystem::path directory = ember::PathFromUtf8(basePath);
    SDL_free(basePath);

    std::error_code error;
    std::vector<std::filesystem::path> projects;
    for (std::filesystem::directory_iterator it(directory, error), end;
         !error && it != end; it.increment(error))
    {
        if (it->is_regular_file(error) && !error && it->path().extension() == ".emberproject")
            projects.push_back(it->path());
        if (error) break;
    }
    return !error && projects.size() == 1 ? projects.front() : std::filesystem::path{};
}
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
{
    const std::filesystem::path projectFile = argc > 1 ? std::filesystem::path(argv[1]) : FindProjectBesideExecutable();
#else
int main(int argc, char** argv)
{
    const std::filesystem::path projectFile = argc > 1 ? ember::PathFromUtf8(argv[1]) : FindProjectBesideExecutable();
#endif
    if (projectFile.empty())
    {
        std::cerr << "EMBER Runtime: pass a project path or place exactly one .emberproject beside the executable.\n";
        return 1;
    }
    ember::RuntimeApplication application;
    if (!application.Initialize(projectFile))
    {
        std::cerr << "EMBER Runtime: failed to initialize project: " << ember::PathToUtf8(projectFile) << "\n";
        return 1;
    }
    return application.Run();
}
