#include "Engine/Scene/SceneSerializer.hpp"
#include "Engine/Physics/PhysicsComponentRegistration.hpp"

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

    Check(ember::RegisterPhysicsComponents(registry),
          "Physics component serializers are registered");

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

    auto& playerHinge = player.AddComponent<ember::RevoluteJoint2D>();
    playerHinge.connectedEntity = second.GetId();
    playerHinge.localAnchorA = {12.0F, 8.0F};
    playerHinge.enableLimit = true;
    playerHinge.lowerAngle = -0.25F;
    playerHinge.upperAngle = 0.75F;
    auto& decorationRope = second.AddComponent<ember::DistanceJoint2D>();
    decorationRope.connectedEntity = player.GetId();
    decorationRope.localAnchorB = {4.0F, 5.0F};
    decorationRope.length = 180.0F;

    auto& compound = source.CreateEntity("Compound");
    auto& colliderSet = compound.AddComponent<ember::ColliderSet2D>();
    auto& boxEntry = colliderSet.AddBoxCollider();
    boxEntry.offset = {3.0F, -4.0F};
    boxEntry.halfExtents = {0.8F, 1.2F};
    boxEntry.density = 2.0F;
    boxEntry.categoryBits = 0x0002;
    const auto boxColliderId = boxEntry.id;
    auto& circleEntry = colliderSet.AddCircleCollider();
    circleEntry.radius = 1.75F;
    circleEntry.isSensor = true;
    circleEntry.maskBits = 0x0004;
    const auto circleColliderId = circleEntry.id;

    const auto scenePath = root / "scenes" / "forest.emberscene";
    std::string error;
    const bool saved = ember::SceneSerializer::Save(source, scenePath, &error, &registry);
    Check(saved, "Scene is saved to a versioned JSON file");
    if (!saved)
    {
        std::cerr << "Scene save error: " << error << '\\n';
        std::filesystem::remove_all(root, filesystemError);
        return 1;
    }
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
    Check(loaded.GetEntityCount() == 3, "All entities survive round-trip");
    const auto* loadedHingeOwner = loaded.FindEntity(player.GetId());
    const auto* loadedRopeOwner = loaded.FindEntity(second.GetId());
    const auto* loadedHinge = loadedHingeOwner
        ? loadedHingeOwner->GetComponent<ember::RevoluteJoint2D>() : nullptr;
    const auto* loadedRope = loadedRopeOwner
        ? loadedRopeOwner->GetComponent<ember::DistanceJoint2D>() : nullptr;
    Check(loadedHinge && loadedHinge->connectedEntity == second.GetId() &&
        loadedHinge->localAnchorA.x == 12.0F && loadedHinge->enableLimit &&
        loadedHinge->lowerAngle == -0.25F && loadedHinge->upperAngle == 0.75F,
        "RevoluteJoint2D reference and configuration survive full Scene serialization");
    Check(loadedRope && loadedRope->connectedEntity == player.GetId() &&
        loadedRope->localAnchorB.y == 5.0F && loadedRope->length == 180.0F,
        "DistanceJoint2D reference and configuration survive full Scene serialization");
    const auto* loadedCompound = loaded.FindEntity(compound.GetId());
    const auto* loadedSet = loadedCompound ? loadedCompound->GetComponent<ember::ColliderSet2D>() : nullptr;
    Check(loadedSet && loadedSet->colliders.size() == 2 &&
        loadedSet->colliders[0].id == boxColliderId &&
        loadedSet->colliders[1].id == circleColliderId &&
        loadedSet->colliders[0].offset.x == 3.0F &&
        loadedSet->colliders[0].density == 2.0F &&
        loadedSet->colliders[0].categoryBits == 0x0002 &&
        loadedSet->colliders[1].radius == 1.75F &&
        loadedSet->colliders[1].isSensor && loadedSet->colliders[1].maskBits == 0x0004,
        "ColliderSet2D survives full Scene serialization with stable IDs and settings");

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

    const auto legacyColliderPath = root / "legacy_box_collider.emberscene";
    ember::Scene legacySource("Legacy Box");
    auto& legacyEntity = legacySource.CreateEntity("Old Box");
    auto& legacyBox = legacyEntity.AddComponent<ember::BoxCollider2D>();
    legacyBox.halfExtents = {2.0F, 3.0F};
    legacyBox.offset = {0.25F, -0.5F};
    legacyBox.rotation = 0.4F;
    legacyBox.density = 2.5F;
    legacyBox.friction = 0.6F;
    legacyBox.restitution = 0.2F;
    legacyBox.isSensor = true;
    Check(ember::SceneSerializer::Save(legacySource, legacyColliderPath, &error, &registry),
        "Legacy collider fixture is written");
    ember::Scene migratedScene("Before Migration");
    Check(ember::SceneSerializer::Load(migratedScene, legacyColliderPath, &error, &registry),
        "Legacy BoxCollider2D scene loads");
    const auto* migratedEntity = migratedScene.FindEntity(legacyEntity.GetId());
    const auto* migratedSet = migratedEntity ? migratedEntity->GetComponent<ember::ColliderSet2D>() : nullptr;
    Check(migratedSet && migratedSet->colliders.size() == 1 &&
        migratedSet->colliders[0].shape == ember::ColliderShape2D::Box &&
        migratedSet->colliders[0].halfExtents.x == 2.0F &&
        migratedSet->colliders[0].halfExtents.y == 3.0F &&
        migratedSet->colliders[0].offset.x == 0.25F &&
        migratedSet->colliders[0].rotation == 0.4F &&
        migratedSet->colliders[0].density == 2.5F &&
        migratedSet->colliders[0].friction == 0.6F &&
        migratedSet->colliders[0].restitution == 0.2F &&
        migratedSet->colliders[0].isSensor &&
        !migratedEntity->HasComponent<ember::BoxCollider2D>(),
        "Legacy BoxCollider2D migrates to ColliderSet2D without losing settings");
    const auto migratedSavePath = root / "migrated_box.emberscene";
    Check(ember::SceneSerializer::Save(migratedScene, migratedSavePath, &error, &registry),
        "Migrated Scene saves");
    nlohmann::json migratedJson;
    { std::ifstream migratedInput(migratedSavePath); migratedJson = nlohmann::json::parse(migratedInput); }
    Check(migratedJson["entities"][0]["components"][0]["type"] == "ember.collider_set_2d",
        "Saving migrated Scene writes canonical ColliderSet2D data");

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
