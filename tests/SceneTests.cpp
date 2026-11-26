#include "Engine/Scene/Scene.hpp"

#include <cstddef>
#include <iostream>
#include <vector>

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
    ember::Scene scene;
    Check(scene.GetName() == "Untitled Scene", "Scene has a default name");
    Check(scene.IsEmpty() && scene.GetEntityCount() == 0, "New scene starts empty");

    scene.SetName("Test Level");
    Check(scene.GetName() == "Test Level", "Scene name can be changed");

    ember::Entity& player = scene.CreateEntity("Player");
    const ember::EntityId playerId = player.GetId();
    Check(playerId != 0, "Created entity receives a non-zero ID");
    Check(player.GetName() == "Player", "Entity keeps its name");
    Check(player.IsActive(), "Entity is active by default");
    Check(player.GetTransform().scale.x == 1.0F &&
          player.GetTransform().scale.y == 1.0F,
          "Entity transform has unit scale by default");

    player.GetTransform().position = {12.0F, 34.0F};
    player.GetTransform().rotation = 0.5F;
    player.GetMovementComponent().enabled = true;
    player.GetMovementComponent().speed = 320.0F;
    auto& visual = player.GetRenderComponent();
    Check(!visual.texture, "Render component starts without a texture");
    visual.texture = ember::TextureHandle{42};
    Check(static_cast<bool>(visual.texture) && visual.texture.id == 42,
          "Render component can reference a texture handle");
    visual.texture = {};
    visual.size = {48.0F, 32.0F};
    visual.red = 20;
    visual.green = 40;
    visual.blue = 60;
    visual.alpha = 200;
    visual.visible = true;

    Check(player.GetTransform().position.x == 12.0F &&
          player.GetTransform().position.y == 34.0F,
          "Entity transform can be updated");
    Check(player.GetTransform().rotation == 0.5F, "Entity rotation can be updated");
    Check(player.GetMovementComponent().enabled && player.GetMovementComponent().speed == 320.0F,
          "Entity movement component stores enabled state and speed");
    Check(visual.visible && visual.size.x == 48.0F && visual.size.y == 32.0F,
          "Render component stores visibility and size");
    Check(visual.red == 20 && visual.green == 40 &&
          visual.blue == 60 && visual.alpha == 200,
          "Render component stores RGBA color");

    ember::Entity& enemy = scene.CreateEntity("Enemy");
    const ember::EntityId enemyId = enemy.GetId();
    Check(enemyId > playerId, "Entity IDs increase monotonically");
    Check(scene.GetEntityCount() == 2, "Scene counts created entities");
    Check(scene.FindEntity(playerId) == &player, "Scene finds entity by ID");
    Check(player.GetName() == "Player", "Entity reference remains stable after insertion");

    const std::vector<ember::EntityId> ids = scene.GetEntityIds();
    Check(ids.size() == 2 && ids[0] == playerId && ids[1] == enemyId,
          "Entity IDs are returned in deterministic order");

    std::size_t visited = 0;
    scene.ForEachEntity([&visited](ember::Entity& entity)
    {
        (void)entity;
        ++visited;
    });
    Check(visited == 2, "Mutable entity iteration visits every entity");

    const ember::Scene& constScene = scene;
    std::size_t constVisited = 0;
    constScene.ForEachEntity([&constVisited](const ember::Entity& entity)
    {
        (void)entity;
        ++constVisited;
    });
    Check(constVisited == 2, "Const entity iteration visits every entity");

    enemy.SetActive(false);
    Check(!enemy.IsActive(), "Entity active state can be disabled");

    Check(scene.DestroyEntity(enemyId), "DestroyEntity removes an existing entity");
    Check(!scene.DestroyEntity(enemyId), "DestroyEntity reports missing entity");
    Check(scene.FindEntity(enemyId) == nullptr, "Destroyed entity cannot be found");
    Check(scene.GetEntityCount() == 1, "Scene count updates after entity destruction");

    scene.Clear();
    Check(scene.IsEmpty(), "Clear removes all entities");

    ember::Entity& afterClear = scene.CreateEntity("AfterClear");
    Check(afterClear.GetId() > enemyId, "Clear does not reuse earlier entity IDs");

    afterClear.GetTransform().position = {15.0F, 25.0F};
    afterClear.GetRenderComponent().visible = true;
    afterClear.GetRenderComponent().size = {90.0F, 45.0F};
    afterClear.GetMovementComponent().enabled = true;
    afterClear.GetMovementComponent().speed = 180.0F;
    afterClear.SetActive(false);
    ember::Scene playCopy = scene.Clone();
    Check(playCopy.GetName() == scene.GetName(), "Scene clone preserves the scene name");
    Check(playCopy.GetEntityCount() == scene.GetEntityCount(), "Scene clone preserves entity count");
    const ember::Entity* copiedEntity = playCopy.FindEntity(afterClear.GetId());
    Check(copiedEntity != nullptr && copiedEntity != &afterClear,
          "Scene clone creates independent entity objects with matching IDs");
    Check(copiedEntity != nullptr &&
          copiedEntity->GetTransform().position.x == 15.0F &&
          copiedEntity->GetTransform().position.y == 25.0F,
          "Scene clone preserves entity transforms");
    Check(copiedEntity != nullptr && !copiedEntity->IsActive() &&
          copiedEntity->GetRenderComponent().visible &&
          copiedEntity->GetRenderComponent().size.x == 90.0F &&
          copiedEntity->GetMovementComponent().enabled &&
          copiedEntity->GetMovementComponent().speed == 180.0F,
          "Scene clone preserves active and render component state");
    if (ember::Entity* copiedMutable = playCopy.FindEntity(afterClear.GetId()))
    {
        copiedMutable->SetName("Changed In Play");
        copiedMutable->GetTransform().position.x = 999.0F;
    }
    Check(afterClear.GetName() == "AfterClear" &&
          afterClear.GetTransform().position.x == 15.0F,
          "Scene clone changes do not modify the source scene");
    const ember::EntityId nextSourceId = scene.CreateEntity("NextSource").GetId();
    const ember::EntityId nextCopyId = playCopy.CreateEntity("NextCopy").GetId();
    Check(nextSourceId == nextCopyId,
          "Scene clone preserves the next entity ID sequence");

    if (g_failures != 0)
    {
        std::cerr << g_failures << " scene test(s) failed.\n";
        return 1;
    }

    std::cout << "All scene tests passed.\n";
    return 0;
}
