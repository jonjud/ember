#include "Runtime/RuntimeApplication.hpp"
#include "Engine/Core/FileSystem.hpp"

#include <iostream>

int main(int argc, char** argv)
{
    const std::filesystem::path projectFile = argc > 1
        ? ember::PathFromUtf8(argv[1])
        : std::filesystem::path("ember.emberproject");

    ember::RuntimeApplication application;
    if (!application.Initialize(projectFile))
    {
        std::cerr << "EMBER Runtime: failed to initialize project.\n";
        return 1;
    }
    return application.Run();
}
