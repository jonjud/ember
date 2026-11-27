#include "Engine/Physics/PhysicsWorld.hpp"

#include <cmath>
#include <iostream>
#include <limits>

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
} // namespace

int main()
{
    ember::PhysicsWorld world;
    Check(world.IsValid(), "PhysicsWorld creates a valid Box2D world");
    Check(world.GetBodyCount() == 0, "New PhysicsWorld starts without bodies");
    Check(!world.Step(0.0F), "PhysicsWorld rejects a zero time step");
    Check(!world.Step(1.0F / 60.0F, 0), "PhysicsWorld rejects a zero sub-step count");
    Check(!world.SetGravity({std::numeric_limits<float>::quiet_NaN(), 0.0F}),
          "PhysicsWorld rejects non-finite gravity");

    ember::PhysicsBodyDefinition2D staticDefinition;
    staticDefinition.type = ember::PhysicsBodyType2D::Static;
    staticDefinition.position = {0.0F, 5.0F};
    const auto ground = world.CreateBody(staticDefinition);
    Check(static_cast<bool>(ground), "PhysicsWorld creates a static body");
    Check(world.AddBoxCollider(ground, {{5.0F, 0.5F}, {}, 0.0F, 0.0F, 0.8F, 0.0F, false}),
          "PhysicsWorld adds a box collider to a body");

    ember::PhysicsBodyDefinition2D dynamicDefinition;
    dynamicDefinition.type = ember::PhysicsBodyType2D::Dynamic;
    dynamicDefinition.position = {0.0F, 0.0F};
    const auto fallingBody = world.CreateBody(dynamicDefinition);
    Check(static_cast<bool>(fallingBody), "PhysicsWorld creates a dynamic body");
    Check(world.AddCircleCollider(fallingBody, {0.35F, {}, 1.0F, 0.4F, 0.0F, false}),
          "PhysicsWorld adds a circle collider to a body");
    Check(!world.AddCircleCollider(fallingBody, {0.0F, {}, 1.0F, 0.4F, 0.0F, false}),
          "PhysicsWorld rejects a zero-radius collider");
    Check(!world.AddBoxCollider({9999}, {}), "PhysicsWorld rejects an unknown body handle");

    const auto rayHit = world.RaycastClosest({-2.0F, 0.0F}, {4.0F, 0.0F});
    Check(rayHit.has_value() && rayHit->body == fallingBody,
        "RaycastClosest returns the nearest body hit");
    Check(rayHit.has_value() && rayHit->normal.x < -0.9F &&
          std::abs(rayHit->fraction - 0.4125F) < 0.03F,
        "RaycastClosest reports hit normal and normalized fraction");
    Check(!world.RaycastClosest({0.0F, 0.0F}, {}).has_value(),
        "RaycastClosest rejects a zero-length ray");
    const auto overlaps = world.OverlapAABB({-0.5F, -0.5F}, {0.5F, 0.5F});
    Check(overlaps.size() == 1 && overlaps.front() == fallingBody,
        "OverlapAABB returns bodies whose shapes overlap the query bounds");
    Check(world.OverlapAABB({1.0F, 1.0F}, {-1.0F, -1.0F}).empty(),
        "OverlapAABB rejects inverted bounds");

    ember::PhysicsQueryFilter2D rejectAllFilter;
    rejectAllFilter.maskBits = 0;
    Check(!world.RaycastClosest({-2.0F, 0.0F}, {4.0F, 0.0F}, rejectAllFilter).has_value(),
        "RaycastClosest respects a query mask that excludes every shape");
    Check(world.OverlapAABB({-0.5F, -0.5F}, {0.5F, 0.5F}, rejectAllFilter).empty(),
        "OverlapAABB respects a query mask that excludes every shape");

    ember::PhysicsQueryFilter2D wrongCategoryFilter;
    wrongCategoryFilter.maskBits = 2;
    Check(!world.RaycastClosest({-2.0F, 0.0F}, {4.0F, 0.0F}, wrongCategoryFilter).has_value(),
        "RaycastClosest rejects shapes outside the query mask category");
    Check(world.OverlapAABB({-0.5F, -0.5F}, {0.5F, 0.5F}, wrongCategoryFilter).empty(),
        "OverlapAABB rejects shapes outside the query mask category");

    ember::PhysicsQueryFilter2D alternateQueryCategory;
    alternateQueryCategory.categoryBits = 2;
    Check(world.RaycastClosest({-2.0F, 0.0F}, {4.0F, 0.0F}, alternateQueryCategory).has_value(),
        "RaycastClosest accepts shapes whose mask allows the query category");
    Check(world.OverlapAABB({-0.5F, -0.5F}, {0.5F, 0.5F}, alternateQueryCategory).size() == 1,
        "OverlapAABB accepts shapes whose mask allows the query category");

    ember::PhysicsBodyDefinition2D sensorDefinition;
    sensorDefinition.type = ember::PhysicsBodyType2D::Static;
    sensorDefinition.position = {10.0F, 0.0F};
    const auto sensorBody = world.CreateBody(sensorDefinition);
    Check(static_cast<bool>(sensorBody), "PhysicsWorld creates a sensor test body");
    Check(world.AddCircleCollider(sensorBody, {0.5F, {}, 0.0F, 0.0F, 0.0F, true}),
        "PhysicsWorld adds a sensor collider");
    const auto sensorRayHit = world.RaycastClosest({8.0F, 0.0F}, {4.0F, 0.0F});
    Check(sensorRayHit.has_value() && sensorRayHit->body == sensorBody,
        "RaycastClosest includes sensor shapes in spatial queries");
    const auto sensorOverlaps = world.OverlapAABB({9.4F, -0.6F}, {10.6F, 0.6F});
    Check(sensorOverlaps.size() == 1 && sensorOverlaps.front() == sensorBody,
        "OverlapAABB includes sensor shapes in spatial queries");

    ember::PhysicsBodyDefinition2D compoundDefinition;
    compoundDefinition.type = ember::PhysicsBodyType2D::Static;
    compoundDefinition.position = {20.0F, 0.0F};
    const auto compoundBody = world.CreateBody(compoundDefinition);
    Check(static_cast<bool>(compoundBody),
        "PhysicsWorld creates one body for multiple collider shapes");

    ember::PhysicsBoxColliderDefinition2D leftBox;
    leftBox.halfExtents = {0.4F, 0.4F};
    leftBox.offset = {-1.0F, 0.0F};
    leftBox.categoryBits = 0x0001;
    leftBox.maskBits = 0xFFFF;
    Check(world.AddBoxCollider(compoundBody, leftBox),
        "PhysicsWorld adds the first independently filtered shape");

    ember::PhysicsCircleColliderDefinition2D rightCircle;
    rightCircle.radius = 0.4F;
    rightCircle.offset = {1.0F, 0.0F};
    rightCircle.categoryBits = 0x0002;
    rightCircle.maskBits = 0xFFFF;
    Check(world.AddCircleCollider(compoundBody, rightCircle),
        "PhysicsWorld adds a second shape to the same body");

    ember::PhysicsQueryFilter2D boxOnlyFilter;
    boxOnlyFilter.maskBits = 0x0001;
    const auto boxOnlyHit = world.RaycastClosest({18.0F, 0.0F}, {2.0F, 0.0F}, boxOnlyFilter);
    Check(boxOnlyHit.has_value() && boxOnlyHit->body == compoundBody &&
          boxOnlyHit->fraction < 0.7F,
        "Raycast query filter selects the Box shape on a compound body");

    ember::PhysicsQueryFilter2D circleOnlyFilter;
    circleOnlyFilter.maskBits = 0x0002;
    const auto circleOnlyHit = world.RaycastClosest({20.0F, 0.0F}, {2.0F, 0.0F}, circleOnlyFilter);
    Check(circleOnlyHit.has_value() && circleOnlyHit->body == compoundBody,
        "Raycast query filter independently selects the Circle shape on the same body");

    const auto compoundOverlaps = world.OverlapAABB({18.5F, -0.5F}, {21.5F, 0.5F});
    Check(compoundOverlaps.size() == 1 && compoundOverlaps.front() == compoundBody,
        "OverlapAABB deduplicates multiple shapes belonging to one body");

    for (int step = 0; step < 180; ++step)
        Check(world.Step(1.0F / 60.0F), "PhysicsWorld advances a fixed simulation step");

    const auto settled = world.GetBodyTransform(fallingBody);
    Check(settled.has_value(), "PhysicsWorld reads a live body transform");
    if (settled)
    {
        Check(settled->position.y > 3.9F && settled->position.y < 4.4F,
              "Dynamic circle settles on the static ground collider");
    }

    Check(world.SetBodyTransform(fallingBody, {{2.0F, 1.0F}, 0.25F}),
          "PhysicsWorld updates a body transform");
    const auto moved = world.GetBodyTransform(fallingBody);
    // Box2D 3.2.0 uses an approximate atan2 for rotation extraction. Allow
    // enough angular error for that approximation while still checking the update.
    constexpr float rotationTolerance = 0.003F;
    Check(moved.has_value() && std::abs(moved->position.x - 2.0F) < 0.001F &&
          std::abs(moved->position.y - 1.0F) < 0.001F &&
          std::abs(moved->rotation - 0.25F) < rotationTolerance,
          "PhysicsWorld returns the updated body transform");

    Check(world.GetBodyCount() == 4, "PhysicsWorld tracks its live body count including a compound body");
    Check(world.DestroyBody(fallingBody), "PhysicsWorld destroys a body");
    Check(!world.DestroyBody(fallingBody), "PhysicsWorld rejects destroying the same handle twice");
    Check(!world.GetBodyTransform(fallingBody).has_value(),
          "Destroyed body handles no longer resolve");
    Check(world.GetBodyCount() == 3, "PhysicsWorld decrements body count after destruction");
    Check(world.DestroyBody(sensorBody), "PhysicsWorld destroys the sensor body");
    Check(world.DestroyBody(ground), "PhysicsWorld destroys the ground body");
    Check(world.DestroyBody(compoundBody), "PhysicsWorld destroys a compound body and all its shapes");
    Check(world.GetBodyCount() == 0, "PhysicsWorld can return to an empty state");

    ember::PhysicsWorld movedWorld(std::move(world));
    Check(movedWorld.IsValid(), "PhysicsWorld transfers ownership through move construction");
    Check(!world.IsValid(), "Moved-from PhysicsWorld is safely empty");

    if (g_failures != 0)
    {
        std::cerr << g_failures << " physics test(s) failed.\n";
        return 1;
    }

    std::cout << "All physics world tests passed.\n";
    return 0;
}
