#pragma once

#include "Engine/Scene/AudioComponents2D.hpp"
#include "Engine/Scene/ComponentRegistry.hpp"

namespace ember
{
inline bool RegisterAudioComponents(ComponentRegistry& registry)
{
    bool ok = registry.Register<AudioSource2D>("ember.audio_source_2d",
        "Audio Source 2D",
        [](const Component& base) {
            const auto& source = static_cast<const AudioSource2D&>(base);
            return nlohmann::json{
                {"audio_asset_id", source.audioAssetId},
                {"volume", source.volume},
                {"loop", source.loop},
                {"play_on_start", source.playOnStart},
                {"streaming", source.streaming},
                {"enabled", source.enabled}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() ||
                !data.contains("audio_asset_id") || !data.at("audio_asset_id").is_string() ||
                !data.contains("volume") || !data.at("volume").is_number() ||
                !data.contains("loop") || !data.at("loop").is_boolean() ||
                !data.contains("play_on_start") || !data.at("play_on_start").is_boolean() ||
                !data.contains("streaming") || !data.at("streaming").is_boolean() ||
                !data.contains("enabled") || !data.at("enabled").is_boolean())
                return false;
            try {
                auto& source = static_cast<AudioSource2D&>(base);
                source.audioAssetId = data.at("audio_asset_id").get<std::string>();
                source.volume = data.at("volume").get<float>();
                source.loop = data.at("loop").get<bool>();
                source.playOnStart = data.at("play_on_start").get<bool>();
                source.streaming = data.at("streaming").get<bool>();
                source.enabled = data.at("enabled").get<bool>();
                return source.IsConfigurationValid();
            } catch (...) { return false; }
        });
    ok = registry.RegisterProperty<AudioSource2D>("ember.audio_source_2d",
        "audio_asset_id", "Audio Asset ID", &AudioSource2D::audioAssetId) && ok;
    ok = registry.RegisterProperty<AudioSource2D>("ember.audio_source_2d",
        "volume", "Volume", &AudioSource2D::volume) && ok;
    ok = registry.RegisterProperty<AudioSource2D>("ember.audio_source_2d",
        "loop", "Loop", &AudioSource2D::loop) && ok;
    ok = registry.RegisterProperty<AudioSource2D>("ember.audio_source_2d",
        "play_on_start", "Play On Start", &AudioSource2D::playOnStart) && ok;
    ok = registry.RegisterProperty<AudioSource2D>("ember.audio_source_2d",
        "streaming", "Streaming", &AudioSource2D::streaming) && ok;
    ok = registry.RegisterProperty<AudioSource2D>("ember.audio_source_2d",
        "enabled", "Enabled", &AudioSource2D::enabled) && ok;
    return ok;
}
} // namespace ember
