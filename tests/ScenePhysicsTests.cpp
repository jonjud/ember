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
    // A moving platform should be kinematic; static bodies are intended to remain fixed.
    groundBody.type = ember::PhysicsBodyType2D::Kinematic;
    auto& groundCollider = ground.AddComponent<ember::BoxCollider2D>();
    groundCollider.halfExtents = {500.0F, 50.0F};
    groundCollider.density = 0.0F;

    auto& ball = scene.CreateEntity("Ball");
    ball.GetTransform().position = {0.0F, 0.0F};
    auto& ballBody = ball.AddComponent<ember::PhysicsBody2D>();
    ballBody.type = ember::PhysicsBodyType2D::Dynamic;
    auto& ballCollider = ball.AddComponent<ember::CircleCollider2D>();
    ballCollider.radius = 35.0F;

    auto& compound = scene.CreateEntity("Compound Collider");
    compound.GetTransform().position = {1000.0F, 0.0F};
    auto& compoundBody = compound.AddComponent<ember::PhysicsBody2D>();
    compoundBody.type = ember::PhysicsBodyType2D::Static;
    auto& compoundSet = compound.AddComponent<ember::ColliderSet2D>();
    auto& compoundBox = compoundSet.AddBoxCollider();
    compoundBox.offset = {-50.0F, 0.0F};
    compoundBox.halfExtents = {20.0F, 20.0F};
    compoundBox.categoryBits = 0x0001;
    auto& compoundCircle = compoundSet.AddCircleCollider();
    compoundCircle.offset = {50.0F, 0.0F};
    compoundCircle.radius = 20.0F;
    compoundCircle.categoryBits = 0x0002;

    Check(scene.GetPhysicsBodyCount() == 0,
        "Physics bodies are not created before Scene Start");
    scene.Start();
    Check(scene.IsStarted(), "Scene enters its running state");
    Check(scene.GetPhysicsBodyCount() == 3,
        "Scene creates one runtime body for each enabled PhysicsBody2D entity, including compound colliders");
    Check(scene.GetPhysicsDiagnostics().empty(),
        "Valid body and collider configuration produces no diagnostics");
    const auto initialRayHit = scene.RaycastClosest({-100.0F, 0.0F}, {200.0F, 0.0F});
    Check(initialRayHit.has_value() && initialRayHit->entity == ball.GetId(),
        "Scene RaycastClosest maps the nearest physics hit to an EntityId");
    const auto initialOverlaps = scene.OverlapAABB({-50.0F, -50.0F}, {50.0F, 50.0F});
    Check(initialOverlaps.size() == 1 && initialOverlaps.front() == ball.GetId(),
        "Scene OverlapAABB maps physics overlaps to EntityIds");

    ember::PhysicsQueryFilter2D boxOnlyFilter;
    boxOnlyFilter.maskBits = 0x0001;
    const auto compoundBoxHit = scene.RaycastClosest(
        {900.0F, 0.0F}, {100.0F, 0.0F}, boxOnlyFilter);
    Check(compoundBoxHit.has_value() && compoundBoxHit->entity == compound.GetId(),
        "Scene creates a Box shape from ColliderSet2D and applies its per-shape query filter");

    ember::PhysicsQueryFilter2D circleOnlyFilter;
    circleOnlyFilter.maskBits = 0x0002;
    const auto compoundCircleHit = scene.RaycastClosest(
        {1000.0F, 0.0F}, {100.0F, 0.0F}, circleOnlyFilter);
    Check(compoundCircleHit.has_value() && compoundCircleHit->entity == compound.GetId(),
        "Scene creates a Circle shape on the same body with an independent query filter");

    const auto compoundOverlaps = scene.OverlapAABB(
        {900.0F, -50.0F}, {1100.0F, 50.0F});
    Check(compoundOverlaps.size() == 1 && compoundOverlaps.front() == compound.GetId(),
        "Scene OverlapAABB deduplicates multiple ColliderSet2D shapes to one EntityId");
    Check(!scene.RaycastClosest({0.0F, 0.0F}, {}).has_value(),
        "Scene RaycastClosest safely rejects a zero-length ray");
    ember::PhysicsQueryFilter2D rejectAllFilter;
    rejectAllFilter.maskBits = 0;
    Check(!scene.RaycastClosest({-100.0F, 0.0F}, {200.0F, 0.0F}, rejectAllFilter).has_value(),
        "Scene RaycastClosest forwards query filtering to PhysicsWorld");
    Check(scene.OverlapAABB({-50.0F, -50.0F}, {50.0F, 50.0F}, rejectAllFilter).empty(),
        "Scene OverlapAABB forwards query filtering to PhysicsWorld");

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
    bool sawEndContact = false;
    // Box2D may defer broad-phase pair removal until the next fixed step after a
    // static-body transform update. Keep collecting the frame-scoped event buffer.
    for (int frame = 0; frame < 3; ++frame)
    {
        scene.Update(1.0F / 60.0F);
        for (const auto& event : scene.GetPhysicsContactEvents())
        {
            if (event.type == ember::SceneContactEventType2D::End &&
                ((event.entityA == ground.GetId() && event.entityB == ball.GetId()) ||
                 (event.entityA == ball.GetId() && event.entityB == ground.GetId())))
                sawEndContact = true;
        }
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

    // Multiple shape-pairs between the same two entities must be exposed as
    // one Entity-level Begin and one End transition.
    ember::Scene compoundContactScene("Compound Contact Aggregation Test");
    auto& compoundPlatform = compoundContactScene.CreateEntity("Compound Platform");
    auto& platformBodyConfig = compoundPlatform.AddComponent<ember::PhysicsBody2D>();
    platformBodyConfig.type = ember::PhysicsBodyType2D::Kinematic;
    auto& platformSet = compoundPlatform.AddComponent<ember::ColliderSet2D>();
    auto& platformLeft = platformSet.AddBoxCollider();
    platformLeft.offset = {-12.0F, 0.0F};
    platformLeft.halfExtents = {20.0F, 20.0F};
    platformLeft.density = 0.0F;
    auto& platformRight = platformSet.AddBoxCollider();
    platformRight.offset = {12.0F, 0.0F};
    platformRight.halfExtents = {20.0F, 20.0F};
    platformRight.density = 0.0F;

    auto& compoundBall = compoundContactScene.CreateEntity("Compound Ball");
    auto& compoundBallBody = compoundBall.AddComponent<ember::PhysicsBody2D>();
    compoundBallBody.type = ember::PhysicsBodyType2D::Dynamic;
    compoundBallBody.gravityScale = 0.0F;
    auto& ballSet = compoundBall.AddComponent<ember::ColliderSet2D>();
    auto& ballLeft = ballSet.AddCircleCollider();
    ballLeft.offset = {-8.0F, 0.0F};
    ballLeft.radius = 25.0F;
    auto& ballRight = ballSet.AddCircleCollider();
    ballRight.offset = {8.0F, 0.0F};
    ballRight.radius = 25.0F;

    compoundContactScene.Start();
    std::size_t compoundBeginCount = 0;
    for (int frame = 0; frame < 5; ++frame)
    {
        compoundContactScene.Update(1.0F / 60.0F);
        for (const auto& event : compoundContactScene.GetPhysicsContactEvents())
            if (event.type == ember::SceneContactEventType2D::Begin &&
                event.entityA == std::min(compoundPlatform.GetId(), compoundBall.GetId()) &&
                event.entityB == std::max(compoundPlatform.GetId(), compoundBall.GetId()))
                ++compoundBeginCount;
    }
    Check(compoundBeginCount == 1,
        "Multiple overlapping shape-pairs emit only one Entity-level Begin event");

    compoundPlatform.GetTransform().position.x = 500.0F;
    std::size_t compoundEndCount = 0;
    for (int frame = 0; frame < 5; ++frame)
    {
        compoundContactScene.Update(1.0F / 60.0F);
        for (const auto& event : compoundContactScene.GetPhysicsContactEvents())
            if (event.type == ember::SceneContactEventType2D::End &&
                event.entityA == std::min(compoundPlatform.GetId(), compoundBall.GetId()) &&
                event.entityB == std::max(compoundPlatform.GetId(), compoundBall.GetId()))
                ++compoundEndCount;
    }
    Check(compoundEndCount == 1,
        "Entity-level End is emitted only after the final shape-pair separates");
    compoundContactScene.Stop();

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


    ember::Scene jointScene("Physics Joint Runtime Test");
    auto& hingeA = jointScene.CreateEntity("Hinge A");
    hingeA.AddComponent<ember::PhysicsBody2D>().type = ember::PhysicsBodyType2D::Static;
    auto& hingeB = jointScene.CreateEntity("Hinge B");
    hingeB.AddComponent<ember::PhysicsBody2D>().type = ember::PhysicsBodyType2D::Dynamic;
    auto& hinge = hingeA.AddComponent<ember::RevoluteJoint2D>();
    hinge.connectedEntity = hingeB.GetId();
    hinge.enableLimit = true;
    hinge.lowerAngle = -0.25F;
    hinge.upperAngle = 0.5F;

    auto& ropeA = jointScene.CreateEntity("Rope A");
    ropeA.AddComponent<ember::PhysicsBody2D>().type = ember::PhysicsBodyType2D::Static;
    auto& ropeB = jointScene.CreateEntity("Rope B");
    ropeB.AddComponent<ember::PhysicsBody2D>().type = ember::PhysicsBodyType2D::Dynamic;
    auto& rope = ropeA.AddComponent<ember::DistanceJoint2D>();
    rope.connectedEntity = ropeB.GetId();
    rope.length = 150.0F;

    auto& invalidJointOwner = jointScene.CreateEntity("Invalid Joint Owner");
    invalidJointOwner.AddComponent<ember::PhysicsBody2D>();
    auto& invalidJoint = invalidJointOwner.AddComponent<ember::DistanceJoint2D>();
    invalidJoint.connectedEntity = 999999;

    auto& selfJointOwner = jointScene.CreateEntity("Self Joint Owner");
    selfJointOwner.AddComponent<ember::PhysicsBody2D>();
    auto& selfJoint = selfJointOwner.AddComponent<ember::RevoluteJoint2D>();
    selfJoint.connectedEntity = selfJointOwner.GetId();

    auto& disabledTarget = jointScene.CreateEntity("Disabled Joint Target");
    disabledTarget.AddComponent<ember::PhysicsBody2D>().enabled = false;
    auto& disabledJointOwner = jointScene.CreateEntity("Disabled Target Joint Owner");
    disabledJointOwner.AddComponent<ember::PhysicsBody2D>();
    auto& disabledJoint = disabledJointOwner.AddComponent<ember::DistanceJoint2D>();
    disabledJoint.connectedEntity = disabledTarget.GetId();

    jointScene.Start();
    Check(jointScene.GetPhysicsBodyCount() == 7,
        "Scene creates bodies before resolving joint references");
    Check(jointScene.GetPhysicsJointCount() == 2,
        "Scene creates valid Revolute and Distance joints from Entity references");
    Check(jointScene.GetPhysicsDiagnostics().size() >= 3,
        "Scene diagnoses missing, self-referencing, and disabled-body joint references");
    jointScene.Stop();
    Check(jointScene.GetPhysicsJointCount() == 0 && jointScene.GetPhysicsBodyCount() == 0,
        "Scene Stop destroys runtime joints and bodies together");
    jointScene.Start();
    Check(jointScene.GetPhysicsJointCount() == 2,
        "Scene Start recreates valid joints after a Stop/Start cycle");
    jointScene.Stop();

    ember::Scene missingBodyScene("Missing Joint Body Test");
    auto& missingOwner = missingBodyScene.CreateEntity("Owner");
    missingOwner.AddComponent<ember::PhysicsBody2D>();
    auto& missingTargetJoint = missingOwner.AddComponent<ember::RevoluteJoint2D>();
    missingTargetJoint.connectedEntity = 12345;
    missingBodyScene.Start();
    Check(missingBodyScene.GetPhysicsJointCount() == 0 &&
        !missingBodyScene.GetPhysicsDiagnostics().empty(),
        "Scene skips and diagnoses a joint with a missing Entity reference");
    missingBodyScene.Stop();

    if (failures != 0)
    {
        std::cerr << failures << " scene physics test(s) failed.\n";
        return 1;
    }
    std::cout << "All Scene physics tests passed.\n";
    return 0;
}
