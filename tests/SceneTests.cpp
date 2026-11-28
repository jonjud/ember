#include "Engine/Scene/Scene.hpp"

#include <cstddef>
#include <cmath>
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

    ember::Scene hierarchyScene;
    ember::Entity& hierarchyRoot = hierarchyScene.CreateEntity("Hierarchy Root");
    ember::Entity& hierarchyChild = hierarchyScene.CreateEntity("Hierarchy Child");
    ember::Entity& hierarchyGrandchild = hierarchyScene.CreateEntity("Hierarchy Grandchild");
    ember::Entity& secondRoot = hierarchyScene.CreateEntity("Second Root");
    hierarchyRoot.GetTransform().position = {100.0F, 50.0F};
    hierarchyRoot.GetTransform().scale = {2.0F, 2.0F};
    hierarchyRoot.GetTransform().rotation = 1.57079632679F;
    hierarchyChild.GetTransform().position = {10.0F, 0.0F};
    secondRoot.GetTransform().position = {300.0F, 100.0F};
    Check(hierarchyScene.SetParent(hierarchyChild.GetId(), hierarchyRoot.GetId(), false),
          "Scene assigns an Entity parent while preserving local transform");
    Check(hierarchyScene.SetParent(hierarchyGrandchild.GetId(), hierarchyChild.GetId(), false),
          "Scene supports nested parent-child relationships");
    const auto childWorld = hierarchyScene.GetWorldTransform(hierarchyChild.GetId());
    Check(childWorld && std::abs(childWorld->position.x - 100.0F) < 0.001F &&
          std::abs(childWorld->position.y - 70.0F) < 0.001F &&
          std::abs(childWorld->scale.x - 2.0F) < 0.001F,
          "Child world transform composes parent position, rotation and scale");
    Check(hierarchyScene.SetParent(hierarchyChild.GetId(), secondRoot.GetId()),
          "Reparenting preserves world transform by default");
    const auto reparentedWorld = hierarchyScene.GetWorldTransform(hierarchyChild.GetId());
    Check(reparentedWorld && childWorld &&
          std::abs(reparentedWorld->position.x - childWorld->position.x) < 0.001F &&
          std::abs(reparentedWorld->position.y - childWorld->position.y) < 0.001F,
          "Keep-world reparenting does not move the child");
    if (reparentedWorld)
    {
        ember::Transform2D desiredWorld = *reparentedWorld;
        desiredWorld.position = {125.0F, 85.0F};
        desiredWorld.scale = {3.0F, 4.0F};
        desiredWorld.rotation = 0.25F;
        Check(hierarchyScene.SetWorldTransform(hierarchyChild.GetId(), desiredWorld),
              "Scene can set a child transform in world space");
        const auto updatedWorld = hierarchyScene.GetWorldTransform(hierarchyChild.GetId());
        Check(updatedWorld && std::abs(updatedWorld->position.x - 125.0F) < 0.001F &&
              std::abs(updatedWorld->position.y - 85.0F) < 0.001F &&
              std::abs(updatedWorld->scale.x - 3.0F) < 0.001F &&
              std::abs(updatedWorld->scale.y - 4.0F) < 0.001F &&
              std::abs(updatedWorld->rotation - 0.25F) < 0.001F,
              "Setting a child world transform correctly updates its local transform");
    }
    Check(hierarchyScene.SetParent(hierarchyChild.GetId(), hierarchyRoot.GetId(), false),
          "Child can be moved back under its original parent");
    Check(!hierarchyScene.SetParent(hierarchyRoot.GetId(), hierarchyGrandchild.GetId()),
          "Scene rejects hierarchy cycles");
    Check(!hierarchyScene.SetParent(hierarchyRoot.GetId(), hierarchyRoot.GetId()),
          "Scene rejects self-parenting");
    Check(!hierarchyScene.SetParent(hierarchyChild.GetId(), 999999),
          "Scene rejects missing parent IDs");
    Check(hierarchyScene.GetRootEntityIds().size() == 2 &&
          hierarchyScene.GetChildren(hierarchyRoot.GetId()).size() == 1 &&
          hierarchyScene.GetChildren(hierarchyChild.GetId()).front() == hierarchyGrandchild.GetId(),
          "Scene returns roots and direct children in deterministic order");
    const ember::EntityId hierarchyRootId = hierarchyRoot.GetId();
    const ember::EntityId hierarchyChildId = hierarchyChild.GetId();
    const ember::EntityId hierarchyGrandchildId = hierarchyGrandchild.GetId();

    player.GetRenderComponent().textureAssetId = "asset-player";
    player.GetRenderComponent().red = 111;
    ember::Entity* duplicate = scene.DuplicateEntity(playerId);
    Check(duplicate != nullptr && duplicate->GetId() > enemyId,
          "DuplicateEntity creates a new entity with a fresh ID");
    Check(duplicate != nullptr && duplicate->GetName() == "Player Copy",
          "DuplicateEntity appends a copy suffix to the entity name");
    Check(duplicate != nullptr &&
          duplicate->GetTransform().position.x == player.GetTransform().position.x &&
          duplicate->GetTransform().rotation == player.GetTransform().rotation &&
          duplicate->GetRenderComponent().textureAssetId == "asset-player" &&
          duplicate->GetRenderComponent().red == 111 &&
          duplicate->GetMovementComponent().enabled &&
          duplicate->GetMovementComponent().speed == 320.0F &&
          duplicate->IsActive() == player.IsActive(),
          "DuplicateEntity copies transform and component state");
    if (duplicate != nullptr)
    {
        duplicate->GetTransform().position.x += 100.0F;
        duplicate->SetName("Independent Copy");
    }
    Check(player.GetTransform().position.x == 12.0F && player.GetName() == "Player",
          "DuplicateEntity data is independent from its source");
    Check(scene.DuplicateEntity(999999) == nullptr,
          "DuplicateEntity returns null for a missing source ID");
    Check(scene.GetEntityCount() == 3, "DuplicateEntity updates scene entity count");
    Check(scene.FindEntity(playerId) == &player, "Scene finds entity by ID");
    Check(player.GetName() == "Player", "Entity reference remains stable after insertion");

    const ember::EntityId duplicateId = duplicate != nullptr ? duplicate->GetId() : 0;
    const std::vector<ember::EntityId> ids = scene.GetEntityIds();
    Check(ids.size() == 3 && ids[0] == playerId && ids[1] == enemyId && ids[2] == duplicateId,
          "Entity IDs are returned in deterministic order");

    std::size_t visited = 0;
    scene.ForEachEntity([&visited](ember::Entity& entity)
    {
        (void)entity;
        ++visited;
    });
    Check(visited == 3, "Mutable entity iteration visits every entity");

    const ember::Scene& constScene = scene;
    std::size_t constVisited = 0;
    constScene.ForEachEntity([&constVisited](const ember::Entity& entity)
    {
        (void)entity;
        ++constVisited;
    });
    Check(constVisited == 3, "Const entity iteration visits every entity");

    enemy.SetActive(false);
    Check(!enemy.IsActive(), "Entity active state can be disabled");

    Check(scene.DestroyEntity(duplicateId),
          "Duplicate entity can be removed independently");
    Check(scene.DestroyEntity(enemyId), "DestroyEntity removes an existing entity");
    const auto childWorldBeforeDelete = hierarchyScene.GetWorldTransform(hierarchyChildId);
    Check(hierarchyScene.DestroyEntity(hierarchyRootId) &&
          hierarchyScene.FindEntity(hierarchyChildId)->GetParentId() == 0 &&
          hierarchyScene.FindEntity(hierarchyGrandchildId)->GetParentId() == hierarchyChildId,
          "Destroying a parent detaches direct children without deleting descendants");
    const auto childWorldAfterDelete = hierarchyScene.GetWorldTransform(hierarchyChildId);
    Check(childWorldBeforeDelete && childWorldAfterDelete &&
          std::abs(childWorldBeforeDelete->position.x - childWorldAfterDelete->position.x) < 0.001F &&
          std::abs(childWorldBeforeDelete->position.y - childWorldAfterDelete->position.y) < 0.001F,
          "Detaching children during parent deletion preserves their world transforms");
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
    ember::Entity& cloneParent = scene.CreateEntity("Clone Parent");
    ember::Entity& cloneChild = scene.CreateEntity("Clone Child");
    Check(scene.SetParent(cloneChild.GetId(), cloneParent.GetId()),
          "Clone hierarchy fixture is parented");
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
    const auto* copiedChild = playCopy.FindEntity(cloneChild.GetId());
    Check(copiedChild != nullptr && copiedChild->GetParentId() == cloneParent.GetId(),
          "Scene clone preserves parent-child relationships");
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
