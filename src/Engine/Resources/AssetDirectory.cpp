#include "Engine/Resources/AssetDirectory.hpp"

#include <utility>

namespace ember
{
AssetDirectory::AssetDirectory(std::filesystem::path root)
    : m_root(std::filesystem::absolute(std::move(root)).lexically_normal())
{
}

const std::filesystem::path& AssetDirectory::GetRoot() const noexcept
{
    return m_root;
}

std::optional<std::filesystem::path> AssetDirectory::Resolve(
    const std::filesystem::path& relativePath) const
{
    if (relativePath.empty() || relativePath.is_absolute() || relativePath.has_root_name())
    {
        return std::nullopt;
    }

    const std::filesystem::path candidate = (m_root / relativePath).lexically_normal();
    const std::filesystem::path relativeToRoot = candidate.lexically_relative(m_root);

    if (relativeToRoot.empty() || *relativeToRoot.begin() == "..")
    {
        return std::nullopt;
    }

    return candidate;
}
} // namespace ember
