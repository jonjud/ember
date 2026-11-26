#include "Engine/Resources/TextureCache.hpp"

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
    ember::TextureCache cache;
    std::unordered_map<std::string, ember::TextureHandle> loaded;
    std::unordered_map<std::uint32_t, int> destroyed;
    std::uint32_t nextId = 1;
    int loadCalls = 0;

    Check(!cache.Load("assets/player.bmp"), "Cache without backend returns an empty handle");
    Check(!cache.SetBackend({}, {}), "Cache rejects an incomplete backend");

    Check(cache.SetBackend(
        [&loaded, &nextId, &loadCalls](const std::string& path)
        {
            ++loadCalls;
            const ember::TextureHandle handle{nextId++};
            loaded.emplace(path, handle);
            return handle;
        },
        [&destroyed](ember::TextureHandle handle)
        {
            ++destroyed[handle.id];
        }),
        "Cache accepts a complete backend");

    const ember::TextureHandle first = cache.Load("assets/./characters/../player.bmp");
    const ember::TextureHandle second = cache.Load("assets/player.bmp");
    Check(first && second && first.id == second.id,
          "Equivalent normalized paths share one texture handle");
    Check(loadCalls == 1, "Repeated loads call the backend only once");
    Check(cache.GetCachedAssetCount() == 1, "Cache tracks one unique asset");
    Check(cache.GetReferenceCount("assets/player.bmp") == 2,
          "Cache tracks the number of active references");

    Check(cache.Release("assets/player.bmp"), "Release decrements an existing reference");
    Check(cache.GetReferenceCount("assets/player.bmp") == 1,
          "One reference remains after the first release");
    Check(destroyed[first.id] == 0, "Texture stays alive while referenced");

    Check(cache.Release("assets/characters/../player.bmp"),
          "Release accepts an equivalent normalized path");
    Check(destroyed[first.id] == 1, "Texture is destroyed when its count reaches zero");
    Check(cache.GetCachedAssetCount() == 0, "Released texture leaves the cache");
    Check(!cache.Release("assets/player.bmp"), "Releasing an unknown asset is reported");

    const ember::TextureHandle failed = cache.Load("");
    Check(!failed, "Empty asset paths are rejected");
    Check(loadCalls == 1, "Invalid paths do not reach the backend");

    const ember::TextureHandle one = cache.Load("assets/one.bmp");
    const ember::TextureHandle two = cache.Load("assets/two.bmp");
    cache.Clear();
    Check(cache.GetCachedAssetCount() == 0, "Clear empties the cache");
    Check(destroyed[one.id] == 1 && destroyed[two.id] == 1,
          "Clear destroys every cached texture exactly once");

    if (g_failures != 0)
    {
        std::cerr << g_failures << " texture cache test(s) failed.\n";
        return 1;
    }

    std::cout << "All texture cache tests passed.\n";
    return 0;
}
