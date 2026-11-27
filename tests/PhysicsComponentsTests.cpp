#include "Engine/Physics/PhysicsComponents2D.hpp"
#include "Engine/Physics/PhysicsComponentRegistration.hpp"

#include <iostream>

namespace
{
int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
    else
    {
        std::cout << "[PASS] " << message << '\n';
    }
}
}

int main()
{
    ember::PhysicsBody2D body;
    body.type = ember::PhysicsBodyType2D::Dynamic;
    body.gravityScale = 2.0F;
    body.enabled = false;
    auto bodyCopy = body.Clone();
    const auto* copiedBody = dynamic_cast<const ember::PhysicsBody2D*>(bodyCopy.get());
    Check(copiedBody && copiedBody->type == ember::PhysicsBodyType2D::Dynamic &&
        copiedBody->gravityScale == 2.0F && !copiedBody->enabled,
        "PhysicsBody2D clones all configuration");

    ember::BoxCollider2D box;
    box.halfExtents = {1.25F, 2.5F};
    box.offset = {0.5F, -0.25F};
    box.isSensor = true;
    auto boxCopy = box.Clone();
    const auto* copiedBox = dynamic_cast<const ember::BoxCollider2D*>(boxCopy.get());
    Check(copiedBox && copiedBox->halfExtents.x == 1.25F &&
        copiedBox->halfExtents.y == 2.5F && copiedBox->offset.y == -0.25F &&
        copiedBox->isSensor, "BoxCollider2D clones geometry and sensor state");

    ember::CircleCollider2D circle;
    circle.radius = 1.75F;
    circle.offset = {0.25F, 0.75F};
    circle.restitution = 0.8F;
    auto circleCopy = circle.Clone();
    const auto* copiedCircle = dynamic_cast<const ember::CircleCollider2D*>(circleCopy.get());
    Check(copiedCircle && copiedCircle->radius == 1.75F &&
        copiedCircle->offset.x == 0.25F && copiedCircle->restitution == 0.8F,
        "CircleCollider2D clones radius, offset, and material");



    ember::ColliderSet2D colliderSet;
    auto& boxEntry = colliderSet.AddBoxCollider();
    boxEntry.offset = {1.0F, -2.0F};
    boxEntry.halfExtents = {0.75F, 1.25F};
    boxEntry.categoryBits = 0x0002;
    const ember::ColliderId boxId = boxEntry.id;
    auto& circleEntry = colliderSet.AddCircleCollider();
    circleEntry.radius = 1.5F;
    circleEntry.isSensor = true;
    const ember::ColliderId circleId = circleEntry.id;
    Check(boxId != 0 && circleId != 0 && boxId != circleId,
        "Collider IDs are nonzero and unique across generated entries");
    Check(colliderSet.colliders.size() == 2 &&
        colliderSet.colliders[0].shape == ember::ColliderShape2D::Box &&
        colliderSet.colliders[1].shape == ember::ColliderShape2D::Circle,
        "ColliderSet2D stores ordered Box and Circle entries");
    Check(colliderSet.HasValidColliderIds(),
        "ColliderSet2D validates nonzero unique collider IDs");
    Check(colliderSet.FindCollider(boxId) != nullptr &&
        colliderSet.FindCollider(boxId)->halfExtents.x == 0.75F &&
        colliderSet.FindCollider(circleId)->radius == 1.5F,
        "ColliderSet2D finds entries by stable ID");
    auto colliderSetCopy = colliderSet.Clone();
    const auto* copiedSet = dynamic_cast<const ember::ColliderSet2D*>(colliderSetCopy.get());
    Check(copiedSet && copiedSet->colliders.size() == 2 &&
        copiedSet->colliders[0].id == boxId && copiedSet->colliders[1].id == circleId &&
        copiedSet->colliders[0].offset.y == -2.0F &&
        copiedSet->colliders[1].isSensor,
        "ColliderSet2D clone preserves IDs and all collider configuration");
    Check(colliderSet.RemoveCollider(boxId) && colliderSet.FindCollider(boxId) == nullptr &&
        colliderSet.colliders.size() == 1,
        "ColliderSet2D removes an entry by stable ID");
    Check(!colliderSet.RemoveCollider(boxId),
        "ColliderSet2D reports when a requested ID does not exist");
    auto& duplicateCheck = colliderSet.AddBoxCollider();
    duplicateCheck.id = circleId;
    Check(!colliderSet.HasValidColliderIds(),
        "ColliderSet2D rejects duplicate IDs");
    duplicateCheck.id = 0;
    Check(!colliderSet.HasValidColliderIds(),
        "ColliderSet2D rejects zero IDs");

    ember::ComponentRegistry registry;
    Check(ember::RegisterPhysicsComponents(registry),
        "Physics components register for Editor creation and serialization");
    Check(registry.Size() == 3, "All three physics component types are registered");

    auto registeredBody = registry.Create("ember.physics_body_2d");
    auto* editableBody = dynamic_cast<ember::PhysicsBody2D*>(registeredBody.get());
    Check(editableBody != nullptr, "Physics Body can be created through the registry");
    if (editableBody)
    {
        const auto* properties = registry.FindProperties(*editableBody);
        Check(properties != nullptr && properties->size() == 5,
            "Physics Body exposes editable Inspector properties");
        nlohmann::json bodyData;
        editableBody->type = ember::PhysicsBodyType2D::Dynamic;
        editableBody->gravityScale = 1.5F;
        Check(registry.Serialize(*editableBody, bodyData),
            "Physics Body configuration serializes");
        auto restored = registry.Create("ember.physics_body_2d", bodyData);
        const auto* restoredBody = dynamic_cast<const ember::PhysicsBody2D*>(restored.get());
        Check(restoredBody && restoredBody->type == ember::PhysicsBodyType2D::Dynamic &&
            restoredBody->gravityScale == 1.5F,
            "Physics Body configuration deserializes");
    }

    auto registeredBox = registry.Create("ember.box_collider_2d");
    auto* editableBox = dynamic_cast<ember::BoxCollider2D*>(registeredBox.get());
    Check(editableBox != nullptr && registry.FindProperties(*editableBox)->size() == 7,
        "Box Collider exposes geometry and material properties");
    auto registeredCircle = registry.Create("ember.circle_collider_2d");
    auto* editableCircle = dynamic_cast<ember::CircleCollider2D*>(registeredCircle.get());
    Check(editableCircle != nullptr && registry.FindProperties(*editableCircle)->size() == 6,
        "Circle Collider exposes radius, offset, and material properties");

    return failures == 0 ? 0 : 1;
}
