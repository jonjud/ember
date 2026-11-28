#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace ember
{
// Writes through a sibling temporary file and atomically replaces the destination.
// When createBackup is true, the previous file is copied to "<destination>.bak".
[[nodiscard]] bool WriteFileAtomically(const std::filesystem::path& destination,
    std::string_view contents, bool createBackup = false, std::string* error = nullptr);
} // namespace ember
