#include "Engine/Physics/PhysicsComponents2D.hpp"
#include "Engine/Scene/Scene.hpp"

#include <cmath>
#include <iostream>
#include <limits>

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
    ember::Scene scene("Physics Runtime Test");

    auto& ground = scene.CreateEntity("Ground");
    ground.GetTransform().position = {0.0F, 500.0F};
    auto& groundBody = ground.AddComponent<ember::PhysicsBody2D>();
    groundBody.type = ember::PhysicsBodyType2D::Static;
    auto& groundCollider = ground.AddComponent<ember::BoxCollider2D>();
    groundCollider.halfExtents = {500.0F, 50.0F};
    groundCollider.density = 0.0F;

    auto& ball = scene.CreateEntity("Ball");
    ball.GetTransform().position = {0.0F, 0.0F};
    auto& ballBody = ball.AddComponent<ember::PhysicsBody2D>();
    ballBody.type = ember::PhysicsBodyType2D::Dynamic;
    auto& ballCollider = ball.AddComponent<ember::CircleCollider2D>();
    ballCollider.radius = 35.0F;

    Check(scene.GetPhysicsBodyCount() == 0,
        "Physics bodies are not created before Scene Start");
    scene.Start();
    Check(scene.IsStarted(), "Scene enters its running state");
    Check(scene.GetPhysicsBodyCount() == 2,
        "Scene creates runtime bodies from PhysicsBody2D components");
    Check(scene.GetPhysicsDiagnostics().empty(),
        "Valid body and collider configuration produces no diagnostics");

    for (int frame = 0; frame < 180; ++frame)
        scene.Update(1.0F / 60.0F);

    const float settledY = ball.GetTransform().position.y;
    Check(settledY > 390.0F && settledY < 450.0F,
        "Fixed-step physics moves the dynamic entity and resolves its ground collision");

    const float beforePause = ball.GetTransform().position.y;
    scene.Update(std::numeric_limits<float>::quiet_NaN());
    Check(std::abs(ball.GetTransform().position.y - beforePause) < 0.001F,
        "Scene ignores non-finite frame delta safely");

    scene.Stop();
    Check(!scene.IsStarted(), "Scene exits its running state");
    Check(scene.GetPhysicsBodyCount() == 0,
        "Scene destroys all runtime physics bodies on Stop");

    ember::Scene playClone = scene.Clone();
    Check(playClone.GetPhysicsBodyCount() == 0,
        "Cloning a stopped Scene copies configuration but no runtime physics handles");

    ember::Scene invalidScene("Invalid Physics Test");
    auto& orphan = invalidScene.CreateEntity("Orphan Collider");
    orphan.AddComponent<ember::BoxCollider2D>();
    auto& ambiguous = invalidScene.CreateEntity("Ambiguous Collider");
    ambiguous.AddComponent<ember::PhysicsBody2D>();
    ambiguous.AddComponent<ember::BoxCollider2D>();
    ambiguous.AddComponent<ember::CircleCollider2D>();
    invalidScene.Start();
    Check(invalidScene.GetPhysicsBodyCount() == 0,
        "Scene skips colliders without a body and ambiguous dual-collider entities");
    Check(invalidScene.GetPhysicsDiagnostics().size() == 2,
        "Scene reports both invalid physics configurations");
    invalidScene.Stop();

    if (failures != 0)
    {
        std::cerr << failures << " scene physics test(s) failed.\n";
        return 1;
    }
    std::cout << "All Scene physics tests passed.\n";
    return 0;
}
