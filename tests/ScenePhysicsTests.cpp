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

    bool sawBeginContact = false;
    for (int frame = 0; frame < 180; ++frame)
    {
        scene.Update(1.0F / 60.0F);
        for (const auto& event : scene.GetPhysicsContactEvents())
        {
            if (event.type == ember::SceneContactEventType2D::Begin &&
                ((event.entityA == ground.GetId() && event.entityB == ball.GetId()) ||
                 (event.entityA == ball.GetId() && event.entityB == ground.GetId())))
                sawBeginContact = true;
        }
    }
    Check(sawBeginContact, "Scene reports a begin-contact event with the correct Entity IDs");

    const float settledY = ball.GetTransform().position.y;
    Check(settledY > 390.0F && settledY < 450.0F,
        "Fixed-step physics moves the dynamic entity and resolves its ground collision");

    ground.GetTransform().position.x = 2000.0F;
    scene.Update(1.0F / 60.0F);
    bool sawEndContact = false;
    for (const auto& event : scene.GetPhysicsContactEvents())
    {
        if (event.type == ember::SceneContactEventType2D::End &&
            ((event.entityA == ground.GetId() && event.entityB == ball.GetId()) ||
             (event.entityA == ball.GetId() && event.entityB == ground.GetId())))
            sawEndContact = true;
    }
    Check(sawEndContact, "Moving a collider apart reports an end-contact event");

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

    ember::Scene validationScene("Physics Validation Test");
    auto& disabled = validationScene.CreateEntity("Disabled Body");
    auto& disabledBody = disabled.AddComponent<ember::PhysicsBody2D>();
    disabledBody.enabled = false;
    disabled.AddComponent<ember::BoxCollider2D>();

    auto& invalidBodyEntity = validationScene.CreateEntity("Invalid Body");
    auto& invalidBody = invalidBodyEntity.AddComponent<ember::PhysicsBody2D>();
    invalidBody.type = ember::PhysicsBodyType2D::Dynamic;
    invalidBody.gravityScale = -1.0F;
    invalidBodyEntity.AddComponent<ember::CircleCollider2D>();

    auto& invalidColliderEntity = validationScene.CreateEntity("Invalid Collider");
    invalidColliderEntity.AddComponent<ember::PhysicsBody2D>();
    auto& invalidCircle = invalidColliderEntity.AddComponent<ember::CircleCollider2D>();
    invalidCircle.radius = -10.0F;

    validationScene.Start();
    Check(validationScene.GetPhysicsBodyCount() == 0,
        "Disabled bodies and invalid body/collider configurations do not leave runtime bodies");
    Check(validationScene.GetPhysicsDiagnostics().size() == 2,
        "Invalid body and collider configurations each produce a diagnostic; disabled bodies are silently skipped");
    validationScene.Stop();

    const auto checkTransformAuthority = [](ember::PhysicsBodyType2D platformType, const char* label)
    {
        ember::Scene authorityScene(label);
        auto& platform = authorityScene.CreateEntity("Platform");
        platform.GetTransform().position = {0.0F, 500.0F};
        auto& platformBody = platform.AddComponent<ember::PhysicsBody2D>();
        platformBody.type = platformType;
        auto& platformCollider = platform.AddComponent<ember::BoxCollider2D>();
        platformCollider.halfExtents = {500.0F, 20.0F};
        platformCollider.density = 0.0F;

        auto& fallingBody = authorityScene.CreateEntity("Falling Body");
        fallingBody.GetTransform().position = {0.0F, 0.0F};
        auto& fallingConfig = fallingBody.AddComponent<ember::PhysicsBody2D>();
        fallingConfig.type = ember::PhysicsBodyType2D::Dynamic;
        auto& fallingCollider = fallingBody.AddComponent<ember::CircleCollider2D>();
        fallingCollider.radius = 20.0F;

        authorityScene.Start();
        platform.GetTransform().position.y = 250.0F;
        for (int frame = 0; frame < 180; ++frame)
            authorityScene.Update(1.0F / 60.0F);

        const float restingY = fallingBody.GetTransform().position.y;
        const bool followsMovedPlatform = restingY > 185.0F && restingY < 235.0F;
        authorityScene.Stop();
        return followsMovedPlatform;
    };

    Check(checkTransformAuthority(ember::PhysicsBodyType2D::Static, "Static Transform Authority"),
        "Static physics bodies follow Entity Transform2D changes before simulation steps");
    Check(checkTransformAuthority(ember::PhysicsBodyType2D::Kinematic, "Kinematic Transform Authority"),
        "Kinematic physics bodies follow Entity Transform2D changes before simulation steps");

    if (failures != 0)
    {
        std::cerr << failures << " scene physics test(s) failed.\n";
        return 1;
    }
    std::cout << "All Scene physics tests passed.\n";
    return 0;
}
