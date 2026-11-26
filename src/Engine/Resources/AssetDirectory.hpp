#pragma once

#include <filesystem>
#include <optional>

namespace ember
{
// Resolves project-relative asset paths beneath a configured asset directory.
// This is a lexical boundary check; it does not resolve or prevent symlink escapes.
class AssetDirectory
{
public:
    explicit AssetDirectory(std::filesystem::path root);

    [[nodiscard]] const std::filesystem::path& GetRoot() const noexcept;
    [[nodiscard]] std::optional<std::filesystem::path> Resolve(
        const std::filesystem::path& relativePath) const;

private:
    std::filesystem::path m_root;
};
} // namespace ember
