#include "Engine/Physics/PhysicsComponents2D.hpp"

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

    return failures == 0 ? 0 : 1;
}
