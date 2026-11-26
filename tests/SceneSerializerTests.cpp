#include "Engine/Scene/SceneSerializer.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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
    const auto root = std::filesystem::temp_directory_path() / "ember_scene_serializer_tests";
    std::error_code filesystemError;
    std::filesystem::remove_all(root, filesystemError);

    ember::Scene source("Forest Level");
    auto& player = source.CreateEntity("Player");
    player.GetTransform().position = {12.5F, -8.0F};
    player.GetTransform().scale = {2.0F, 1.5F};
    player.GetTransform().rotation = 0.75F;
    player.SetActive(false);
    player.GetMovementComponent().enabled = true;
    player.GetMovementComponent().speed = 375.0F;

    auto& visual = player.GetRenderComponent();
    visual.size = {48.0F, 32.0F};
    visual.red = 20;
    visual.green = 40;
    visual.blue = 60;
    visual.alpha = 200;
    visual.visible = true;
    visual.textureAssetId = "asset-player-texture";
    visual.texture = ember::TextureHandle{999};

    auto& second = source.CreateEntity("Decoration");
    second.GetTransform().position = {100.0F, 200.0F};

    const auto scenePath = root / "scenes" / "forest.emberscene";
    std::string error;
    Check(ember::SceneSerializer::Save(source, scenePath, &error),
          "Scene is saved to a versioned JSON file");

    ember::Scene loaded("Old Scene");
    loaded.CreateEntity("Will Be Replaced");
    Check(ember::SceneSerializer::Load(loaded, scenePath, &error),
          "Saved scene can be loaded");
    Check(loaded.GetName() == "Forest Level", "Scene name survives round-trip");
    Check(loaded.GetEntityCount() == 2, "All entities survive round-trip");

    const auto* loadedPlayer = loaded.FindEntity(player.GetId());
    Check(loadedPlayer != nullptr, "Entity IDs are preserved");
    if (loadedPlayer != nullptr)
    {
        const auto& transform = loadedPlayer->GetTransform();
        const auto& loadedVisual = loadedPlayer->GetRenderComponent();
        Check(loadedPlayer->GetName() == "Player" && !loadedPlayer->IsActive(),
              "Entity name and active state survive round-trip");
        Check(transform.position.x == 12.5F && transform.position.y == -8.0F &&
              transform.scale.x == 2.0F && transform.scale.y == 1.5F &&
              transform.rotation == 0.75F,
              "Transform values survive round-trip");
        Check(loadedVisual.size.x == 48.0F && loadedVisual.size.y == 32.0F &&
              loadedVisual.red == 20 && loadedVisual.green == 40 &&
              loadedVisual.blue == 60 && loadedVisual.alpha == 200 &&
              loadedVisual.visible,
              "Render component values survive round-trip");
        Check(loadedPlayer->GetMovementComponent().enabled &&
              loadedPlayer->GetMovementComponent().speed == 375.0F,
              "Movement component settings survive round-trip");
        Check(loadedVisual.textureAssetId == "asset-player-texture",
              "Stable texture asset ID survives round-trip");
        Check(!loadedVisual.texture,
              "Runtime texture handles are not restored from serialized data");
    }

    const auto nextEntityId = loaded.CreateEntity("Next").GetId();
    Check(nextEntityId > second.GetId(), "New entity IDs continue after deserialization");

    const auto oldName = loaded.GetName();
    const auto oldCount = loaded.GetEntityCount();
    const std::string duplicateEntity =
        R"({"id":1,"name":"Duplicate","active":true,"transform":{"position":[0,0],"scale":[1,1],"rotation":0},"render":{"size":[16,16],"tint":[255,255,255,255],"visible":true,"texture_asset_id":""}})";
    const auto duplicatePath = root / "duplicate.emberscene";
    {
        std::ofstream duplicate(duplicatePath);
        duplicate << R"({"schema_version":1,"scene_name":"Duplicate IDs","entities":[)"
                  << duplicateEntity << "," << duplicateEntity << R"(]})";
    }
    Check(!ember::SceneSerializer::Load(loaded, duplicatePath, &error),
          "Duplicate entity IDs are rejected");
    Check(loaded.GetName() == oldName && loaded.GetEntityCount() == oldCount,
          "Failed duplicate-ID load leaves the destination scene unchanged");

    const auto unsupportedPath = root / "unsupported.emberscene";
    {
        std::ofstream unsupported(unsupportedPath);
        unsupported << R"({"schema_version":999,"scene_name":"Future","entities":[]})";
    }
    Check(!ember::SceneSerializer::Load(loaded, unsupportedPath, &error),
          "Unsupported scene schema versions are rejected");

    const auto negativeSizePath = root / "negative_size.emberscene";
    {
        std::ofstream negativeSize(negativeSizePath);
        negativeSize << R"({"schema_version":1,"scene_name":"Negative Size","entities":[{"id":1,"name":"Bad Size","active":true,"transform":{"position":[0,0],"scale":[1,1],"rotation":0},"render":{"size":[-1,16],"tint":[255,255,255,255],"visible":true,"texture_asset_id":""}}]})";
    }
    Check(!ember::SceneSerializer::Load(loaded, negativeSizePath, &error),
          "Negative render dimensions are rejected");

    std::filesystem::remove_all(root, filesystemError);

    if (g_failures != 0)
    {
        std::cerr << g_failures << " scene serializer test(s) failed.\n";
        return 1;
    }

    std::cout << "All scene serializer tests passed.\n";
    return 0;
}
