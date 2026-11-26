#include "Engine/Resources/AssetDirectory.hpp"

#include <filesystem>
#include <iostream>

namespace
{
int g_failures = 0;

void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << description << '\n';
        ++g_failures;
    }
    else
    {
        std::cout << "[PASS] " << description << '\n';
    }
}
}

int main()
{
    const std::filesystem::path root =
        std::filesystem::current_path() / "EMBER_Test_Project" / "assets";
    const ember::AssetDirectory assets(root);

    const auto sprite = assets.Resolve("sprites/player.bmp");
    Check(sprite.has_value(), "Asset directory resolves a relative sprite path");
    Check(sprite && *sprite == (assets.GetRoot() / "sprites/player.bmp").lexically_normal(),
          "Resolved asset path stays beneath the configured root");

    const auto normalized = assets.Resolve("sprites/../textures/tiles.bmp");
    Check(normalized && *normalized == (assets.GetRoot() / "textures/tiles.bmp").lexically_normal(),
          "Asset directory normalizes internal parent segments");

    Check(!assets.Resolve("../settings.ini"), "Asset directory rejects parent traversal");
    Check(!assets.Resolve(""), "Asset directory rejects an empty path");
    Check(!assets.Resolve(assets.GetRoot() / "outside.bmp"),
          "Asset directory rejects absolute paths");

    if (g_failures != 0)
    {
        std::cerr << g_failures << " asset directory test(s) failed.\n";
        return 1;
    }

    std::cout << "All asset directory tests passed.\n";
    return 0;
}
