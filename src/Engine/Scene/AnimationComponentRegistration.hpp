#pragma once

#include "Engine/Scene/AnimationComponents2D.hpp"
#include "Engine/Scene/ComponentRegistry.hpp"

#include <cmath>

namespace ember
{
inline bool RegisterAnimationComponents(ComponentRegistry& registry)
{
    bool ok = registry.Register<SpriteAnimation2D>("ember.sprite_animation_2d",
        "Sprite Animation 2D",
        [](const Component& base) {
            const auto& animation = static_cast<const SpriteAnimation2D&>(base);
            return nlohmann::json{{"frame_width", animation.frameWidth},
                {"frame_height", animation.frameHeight}, {"columns", animation.columns},
                {"frame_count", animation.frameCount}, {"frames_per_second", animation.framesPerSecond},
                {"loop", animation.loop}, {"playing", animation.playing}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() || !data.contains("frame_width") ||
                !data.at("frame_width").is_number_integer() ||
                !data.contains("frame_height") || !data.at("frame_height").is_number_integer() ||
                !data.contains("columns") || !data.at("columns").is_number_integer() ||
                !data.contains("frame_count") || !data.at("frame_count").is_number_integer() ||
                !data.contains("frames_per_second") || !data.at("frames_per_second").is_number() ||
                !data.contains("loop") || !data.at("loop").is_boolean() ||
                !data.contains("playing") || !data.at("playing").is_boolean()) return false;
            try {
                auto& animation = static_cast<SpriteAnimation2D&>(base);
                animation.frameWidth = data.at("frame_width").get<int>();
                animation.frameHeight = data.at("frame_height").get<int>();
                animation.columns = data.at("columns").get<int>();
                animation.frameCount = data.at("frame_count").get<int>();
                animation.framesPerSecond = data.at("frames_per_second").get<float>();
                animation.loop = data.at("loop").get<bool>();
                animation.playing = data.at("playing").get<bool>();
                return animation.IsConfigurationValid();
            } catch (...) { return false; }
        });
    ok = registry.RegisterProperty<SpriteAnimation2D>("ember.sprite_animation_2d",
        "frame_width", "Frame Width (px)", &SpriteAnimation2D::frameWidth) && ok;
    ok = registry.RegisterProperty<SpriteAnimation2D>("ember.sprite_animation_2d",
        "frame_height", "Frame Height (px)", &SpriteAnimation2D::frameHeight) && ok;
    ok = registry.RegisterProperty<SpriteAnimation2D>("ember.sprite_animation_2d",
        "columns", "Columns", &SpriteAnimation2D::columns) && ok;
    ok = registry.RegisterProperty<SpriteAnimation2D>("ember.sprite_animation_2d",
        "frame_count", "Frame Count", &SpriteAnimation2D::frameCount) && ok;
    ok = registry.RegisterProperty<SpriteAnimation2D>("ember.sprite_animation_2d",
        "frames_per_second", "Frames Per Second", &SpriteAnimation2D::framesPerSecond) && ok;
    ok = registry.RegisterProperty<SpriteAnimation2D>("ember.sprite_animation_2d",
        "loop", "Loop", &SpriteAnimation2D::loop) && ok;
    ok = registry.RegisterProperty<SpriteAnimation2D>("ember.sprite_animation_2d",
        "playing", "Playing", &SpriteAnimation2D::playing) && ok;
    return ok;
}
} // namespace ember
