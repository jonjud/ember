#include "Engine/Scene/SceneSerializer.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <memory>

namespace
{
class SerializableTestComponent final : public ember::Component
{
public:
    int value = 17;
    std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<SerializableTestComponent>(*this);
    }
};

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

    ember::ComponentRegistry registry;
    registry.Register<SerializableTestComponent>("test.serializable", "Serializable Test",
        [](const ember::Component& component) -> nlohmann::json {
            return nlohmann::json{{"amount", static_cast<const SerializableTestComponent&>(component).value}};
        },
        [](ember::Component& component, const nlohmann::json& data) {
            if (!data.is_object() || !data.contains("amount") || !data.at("amount").is_number_integer()) return false;
            const int value = data.at("amount").get<int>();
            if (value < 0 || value > 1000) return false;
            static_cast<SerializableTestComponent&>(component).value = value;
            return true;
        }, 2);
    Check(registry.RegisterMigration("test.serializable", 1,
        [](nlohmann::json& data) {
            if (!data.is_object() || !data.contains("value") ||
                !data.at("value").is_number_integer()) return false;
            data["amount"] = data["value"];
            data.erase("value");
            return true;
        }), "Legacy generic component migration is registered");

    ember::Scene source("Forest Level");
    auto& player = source.CreateEntity("Player");
    player.GetTransform().position = {12.5F, -8.0F};
    player.GetTransform().scale = {2.0F, 1.5F};
    player.GetTransform().rotation = 0.75F;
    player.SetActive(false);
    player.GetMovementComponent().enabled = true;
    player.GetMovementComponent().speed = 375.0F;
    player.AddComponent<SerializableTestComponent>().value = 73;

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
    Check(ember::SceneSerializer::Save(source, scenePath, &error, &registry),
          "Scene is saved to a versioned JSON file");
    nlohmann::json savedDocument;
    {
        std::ifstream savedInput(scenePath);
        savedDocument = nlohmann::json::parse(savedInput);
    }
    Check(savedDocument.at("entities").at(0).at("components").at(0)
              .at("version").get<std::uint32_t>() == 2,
          "Saved component payload includes its schema version");

    ember::Scene loaded("Old Scene");
    loaded.CreateEntity("Will Be Replaced");
    Check(ember::SceneSerializer::Load(loaded, scenePath, &error, &registry),
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
        const auto* generic = loadedPlayer->GetComponent<SerializableTestComponent>();
        Check(generic != nullptr && generic->value == 73,
              "Registered generic component data survives scene round-trip");
        Check(loadedPlayer->GetMovementComponent().enabled &&
              loadedPlayer->GetMovementComponent().speed == 375.0F,
              "Movement component settings survive round-trip");
        Check(loadedVisual.textureAssetId == "asset-player-texture",
              "Stable texture asset ID survives round-trip");
        Check(!loadedVisual.texture,
              "Runtime texture handles are not restored from serialized data");
    }

    const auto legacyPath = root / "legacy_component.emberscene";
    auto legacyDocument = savedDocument;
    auto& legacyPayload = legacyDocument["entities"][0]["components"][0];
    legacyPayload.erase("version"); // Files from the previous format had no component version.
    legacyPayload["data"]["value"] = legacyPayload["data"]["amount"];
    legacyPayload["data"].erase("amount");
    {
        std::ofstream legacyOutput(legacyPath);
        legacyOutput << legacyDocument.dump(4);
    }
    ember::Scene legacyLoaded("Legacy Scene");
    Check(ember::SceneSerializer::Load(legacyLoaded, legacyPath, &error, &registry),
          "Versionless legacy component payloads load through the v1 migration");
    const auto* legacyPlayer = legacyLoaded.FindEntity(player.GetId());
    const auto* legacyComponent = legacyPlayer
        ? legacyPlayer->GetComponent<SerializableTestComponent>() : nullptr;
    Check(legacyComponent != nullptr && legacyComponent->value == 73,
          "Legacy component values survive schema migration");

    const auto futureComponentPath = root / "future_component.emberscene";
    auto futureDocument = savedDocument;
    futureDocument["entities"][0]["components"][0]["version"] = 999;
    {
        std::ofstream futureOutput(futureComponentPath);
        futureOutput << futureDocument.dump(4);
    }
    const auto sceneBeforeFutureLoad = loaded.GetName();
    Check(!ember::SceneSerializer::Load(loaded, futureComponentPath, &error, &registry),
          "Future component schema versions are rejected by scene loading");
    Check(loaded.GetName() == sceneBeforeFutureLoad,
          "Failed future-component load leaves the destination scene unchanged");

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
