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
    Check(moved.has_value() && std::abs(moved->position.x - 2.0F) < 0.001F &&
          std::abs(moved->position.y - 1.0F) < 0.001F &&
          std::abs(moved->rotation - 0.25F) < 0.001F,
          "PhysicsWorld returns the updated body transform");

    Check(world.GetBodyCount() == 2, "PhysicsWorld tracks its live body count");
    Check(world.DestroyBody(fallingBody), "PhysicsWorld destroys a body");
    Check(!world.DestroyBody(fallingBody), "PhysicsWorld rejects destroying the same handle twice");
    Check(!world.GetBodyTransform(fallingBody).has_value(),
          "Destroyed body handles no longer resolve");
    Check(world.GetBodyCount() == 1, "PhysicsWorld decrements body count after destruction");
    Check(world.DestroyBody(ground), "PhysicsWorld destroys the remaining body");
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
