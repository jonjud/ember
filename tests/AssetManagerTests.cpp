#include "Engine/Resources/AssetManager.hpp"

#include <iostream>
#include <string>
#include <unordered_map>

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
    ember::AssetManager assets("TestProject/assets");
    std::unordered_map<std::uint32_t, int> destroyed;
    std::uint32_t nextId = 1;
    int loadCalls = 0;

    Check(assets.SetTextureBackend(
        [&nextId, &loadCalls](const std::string& path)
        {
            ++loadCalls;
            Check(path.find("sprites/player.bmp") != std::string::npos,
                  "Backend receives the resolved project asset path");
            return ember::TextureHandle{nextId++};
        },
        [&destroyed](ember::TextureHandle handle)
        {
            ++destroyed[handle.id];
        }), "Asset manager accepts texture backend callbacks");

    const auto player = assets.LoadTexture("sprites/player.bmp");
    const auto samePlayer = assets.LoadTexture("sprites/./player.bmp");
    Check(player && samePlayer && player.id == samePlayer.id,
          "Asset manager shares equivalent relative texture paths");
    Check(loadCalls == 1, "Repeated asset requests load the texture once");
    Check(assets.GetCachedTextureCount() == 1, "Asset manager reports cached texture count");

    Check(!assets.LoadTexture("../outside.bmp"),
          "Asset manager rejects paths escaping the project asset directory");
    Check(!assets.LoadTexture(""),
          "Asset manager rejects empty asset paths");
    Check(loadCalls == 1, "Rejected paths never reach the renderer backend");

    Check(assets.ReleaseTexture("sprites/player.bmp"),
          "Asset manager releases one acquired texture reference");
    Check(destroyed[player.id] == 0, "Texture remains alive while another reference exists");
    Check(assets.ReleaseTexture("sprites/./player.bmp"),
          "Equivalent path can release the remaining reference");
    Check(destroyed[player.id] == 1, "Texture is destroyed after the final release");
    Check(assets.GetCachedTextureCount() == 0, "Released texture is removed from cache");

    if (g_failures != 0)
    {
        std::cerr << g_failures << " asset manager test(s) failed.\n";
        return 1;
    }

    std::cout << "All asset manager tests passed.\n";
    return 0;
}
