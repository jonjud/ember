#include "Engine/Scene/AnimationComponents2D.hpp"
#include "Engine/Scene/Scene.hpp"

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
    ember::Scene scene("Sprite Animation Test");
    auto& entity = scene.CreateEntity("Animated Sprite");
    auto& animation = entity.AddComponent<ember::SpriteAnimation2D>();
    animation.frameWidth = 16;
    animation.frameHeight = 24;
    animation.columns = 4;
    animation.frameCount = 4;
    animation.framesPerSecond = 10.0F;
    animation.loop = true;

    scene.Start();
    Check(animation.GetCurrentFrame() == 0, "Animation starts from the first frame");
    scene.Update(0.05F);
    Check(animation.GetCurrentFrame() == 0, "Animation accumulates partial frame time");
    scene.Update(0.06F);
    Check(animation.GetCurrentFrame() == 1, "Animation advances after one frame duration");
    scene.Update(0.25F);
    Check(animation.GetCurrentFrame() == 3, "Animation advances multiple frames for a larger delta");
    scene.Update(0.1F);
    Check(animation.GetCurrentFrame() == 0, "Looping animation wraps to the first frame");

    animation.loop = false;
    animation.Stop();
    animation.Play();
    scene.Update(0.25F);
    Check(animation.GetCurrentFrame() == 2 && animation.playing,
        "Non-looping animation advances without wrapping before its last frame");
    scene.Update(0.2F);
    Check(animation.GetCurrentFrame() == 3 && !animation.playing,
        "Non-looping animation stops on its final frame");

    animation.frameWidth = 0;
    animation.playing = true;
    scene.Update(0.2F);
    Check(!animation.IsConfigurationValid() && animation.GetCurrentFrame() == 3,
        "Invalid frame dimensions safely prevent playback advancement");

    auto clonedBase = animation.Clone();
    const auto* cloned = dynamic_cast<const ember::SpriteAnimation2D*>(clonedBase.get());
    Check(cloned && cloned->frameWidth == 0 && cloned->frameCount == 4 &&
        cloned->GetCurrentFrame() == 0,
        "Animation clone preserves authoring configuration but resets runtime playback state");

    scene.Stop();
    if (failures != 0)
    {
        std::cerr << failures << " sprite animation test(s) failed.\n";
        return 1;
    }
    std::cout << "All sprite animation tests passed.\n";
    return 0;
}
