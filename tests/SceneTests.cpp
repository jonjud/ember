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
    auto& visual = player.GetRenderComponent();
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

    if (g_failures != 0)
    {
        std::cerr << g_failures << " scene test(s) failed.\n";
        return 1;
    }

    std::cout << "All scene tests passed.\n";
    return 0;
}
