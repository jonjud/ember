#include "Engine/Audio/AudioEngine.hpp"
#include "Engine/Resources/AssetMetadataStore.hpp"
#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Scene/AudioComponents2D.hpp"
#include "Engine/Scene/Scene.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <utility>

namespace
{
void WriteTinyWave(const std::filesystem::path& path)
{
    constexpr std::uint32_t sampleRate = 8000;
    constexpr std::uint16_t channels = 1;
    constexpr std::uint16_t bitsPerSample = 16;
    constexpr std::uint32_t sampleCount = 800;
    constexpr std::uint32_t dataBytes = sampleCount * sizeof(std::int16_t);
    const std::uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    const std::uint16_t blockAlign = channels * bitsPerSample / 8;
    std::ofstream out(path, std::ios::binary);
    const auto write = [&out](const auto& value) {
        out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    };
    out.write("RIFF", 4);
    const std::uint32_t riffSize = 36 + dataBytes;
    write(riffSize);
    out.write("WAVEfmt ", 8);
    const std::uint32_t fmtSize = 16;
    write(fmtSize);
    const std::uint16_t format = 1;
    write(format);
    write(channels);
    write(sampleRate);
    write(byteRate);
    write(blockAlign);
    write(bitsPerSample);
    out.write("data", 4);
    write(dataBytes);
    for (std::uint32_t i = 0; i < sampleCount; ++i) {
        const std::int16_t sample = static_cast<std::int16_t>(
            std::sin(static_cast<double>(i) * 0.15) * 10000.0);
        write(sample);
    }
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "ember_audio_scene_tests";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root / "audio", error);
    const auto wav = root / "audio" / "ambience.wav";
    WriteTinyWave(wav);

    ember::AssetMetadata metadata;
    metadata.assetId = "audio-ambience";
    metadata.type = ember::AssetType::Audio;
    metadata.source = "audio/ambience.wav";
    assert(ember::AssetMetadataStore::Save(root / "audio" / "ambience.emberasset", metadata));

    ember::AssetRegistry assets(root);
    assert(assets.Scan());

    ember::AudioEngine audio;
    assert(audio.Initialize({true, 2, 48000}));

    ember::Scene scene("Audio Scene");
    auto& ambienceEntity = scene.CreateEntity("Ambience");
    auto& ambience = ambienceEntity.AddComponent<ember::AudioSource2D>();
    ambience.audioAssetId = "audio-ambience";
    ambience.loop = true;
    ambience.volume = 0.35F;

    auto& delayedEntity = scene.CreateEntity("Delayed Sound");
    auto& delayed = delayedEntity.AddComponent<ember::AudioSource2D>();
    delayed.audioAssetId = "audio-ambience";
    delayed.playOnStart = false;

    auto& disabledEntity = scene.CreateEntity("Disabled Sound");
    auto& disabled = disabledEntity.AddComponent<ember::AudioSource2D>();
    disabled.audioAssetId = "audio-ambience";
    disabled.enabled = false;

    scene.Start(&audio, &assets);
    assert(scene.IsStarted());
    assert(scene.GetAudioSourceCount() == 2);
    assert(scene.IsAudioSourcePlaying(ambienceEntity.GetId()));
    assert(!scene.IsAudioSourcePlaying(delayedEntity.GetId()));
    assert(scene.PlayAudioSource(delayedEntity.GetId()));
    assert(scene.IsAudioSourcePlaying(delayedEntity.GetId()));
    assert(scene.StopAudioSource(delayedEntity.GetId()));
    assert(!scene.IsAudioSourcePlaying(delayedEntity.GetId()));
    assert(!scene.PlayAudioSource(0));
    assert(!scene.StopAudioSource(0));
    assert(!scene.IsAudioSourcePlaying(disabledEntity.GetId()));
    assert(scene.GetAudioDiagnostics().empty());

    scene.Stop();
    assert(!scene.IsStarted());
    assert(scene.GetAudioSourceCount() == 0);
    assert(!scene.IsAudioSourcePlaying(ambienceEntity.GetId()));

    scene.Start(&audio, &assets);
    assert(scene.GetAudioSourceCount() == 2);
    scene.Stop();

    ember::Scene missingAssetScene("Missing Audio");
    auto& missing = missingAssetScene.CreateEntity("Missing");
    auto& missingSource = missing.AddComponent<ember::AudioSource2D>();
    missingSource.audioAssetId = "not-found";
    missingAssetScene.Start(&audio, &assets);
    assert(missingAssetScene.GetAudioSourceCount() == 0);
    assert(!missingAssetScene.GetAudioDiagnostics().empty());
    missingAssetScene.Stop();

    // Moving a running Scene transfers ownership of its runtime sound handles.
    ember::Scene moveSource("Move Source");
    auto& movingEntity = moveSource.CreateEntity("Moving Audio");
    auto& movingSource = movingEntity.AddComponent<ember::AudioSource2D>();
    movingSource.audioAssetId = "audio-ambience";
    movingSource.loop = true;
    moveSource.Start(&audio, &assets);
    assert(moveSource.IsAudioSourcePlaying(movingEntity.GetId()));

    ember::Scene moveTarget("Move Target");
    auto& replacedEntity = moveTarget.CreateEntity("Replaced Audio");
    auto& replacedSource = replacedEntity.AddComponent<ember::AudioSource2D>();
    replacedSource.audioAssetId = "audio-ambience";
    moveTarget.Start(&audio, &assets);
    assert(moveTarget.IsAudioSourcePlaying(replacedEntity.GetId()));

    moveTarget = std::move(moveSource);
    assert(!moveSource.IsStarted());
    assert(moveTarget.IsStarted());
    assert(moveTarget.GetAudioSourceCount() == 1);
    assert(moveTarget.IsAudioSourcePlaying(movingEntity.GetId()));
    assert(!moveTarget.IsAudioSourcePlaying(replacedEntity.GetId()));
    moveTarget.Stop();
    assert(!moveTarget.IsAudioSourcePlaying(movingEntity.GetId()));

    audio.Shutdown();
    std::filesystem::remove_all(root, error);
    return 0;
}
