#include "Engine/Core/FileSystem.hpp"

#include <fstream>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace ember
{
std::string PathToUtf8(const std::filesystem::path& path)
{
    const auto encoded = path.generic_u8string();
    return std::string(reinterpret_cast<const char*>(encoded.data()), encoded.size());
}

std::filesystem::path PathFromUtf8(std::string_view path)
{
    return std::filesystem::u8path(path.begin(), path.end());
}

bool WriteFileAtomically(const std::filesystem::path& destination,
    std::string_view contents, bool createBackup, std::string* error)
{
    const auto fail = [error](const char* message)
    {
        if (error != nullptr) *error = message;
        return false;
    };
    if (destination.empty()) return fail("Destination path is empty.");

    std::error_code filesystemError;
    const auto parent = destination.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, filesystemError);
        if (filesystemError) return fail("Could not create destination directory.");
    }

    std::filesystem::path temporary = destination;
    temporary += ".ember-tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return fail("Could not open temporary file for writing.");
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.flush();
        if (!output)
        {
            output.close();
            std::filesystem::remove(temporary, filesystemError);
            return fail("Could not write complete temporary file.");
        }
    }

    if (createBackup && std::filesystem::exists(destination, filesystemError) && !filesystemError)
    {
        std::filesystem::path backup = destination;
        backup += ".bak";
        std::filesystem::copy_file(destination, backup,
            std::filesystem::copy_options::overwrite_existing, filesystemError);
        if (filesystemError)
        {
            std::filesystem::remove(temporary, filesystemError);
            return fail("Could not create backup of the existing file.");
        }
    }

#ifdef _WIN32
    const bool replaced = MoveFileExW(temporary.c_str(), destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::filesystem::rename(temporary, destination, filesystemError);
    const bool replaced = !filesystemError;
#endif
    if (!replaced)
    {
        std::filesystem::remove(temporary, filesystemError);
        return fail("Could not atomically replace destination file.");
    }
    if (error != nullptr) error->clear();
    return true;
}
} // namespace ember
