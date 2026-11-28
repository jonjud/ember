#include "Engine/Audio/AudioEngine.hpp"
#include "Engine/Resources/AssetMetadataStore.hpp"
#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Scene/AudioComponents2D.hpp"
#include "Engine/Scene/Scene.hpp"

#include "TestSupport.hpp"
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
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
    EMBER_CHECK(ember::AssetMetadataStore::Save(root / "audio" / "ambience.emberasset", metadata));

    ember::AssetRegistry assets(root);
    EMBER_CHECK(assets.Scan());

    ember::AudioEngine audio;
    EMBER_CHECK(audio.Initialize({true, 2, 48000}));

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

    auto& inactiveEntity = scene.CreateEntity("Inactive Sound");
    auto& inactive = inactiveEntity.AddComponent<ember::AudioSource2D>();
    inactive.audioAssetId = "audio-ambience";
    inactiveEntity.SetActive(false);

    scene.Start(&audio, &assets);
    EMBER_CHECK(scene.IsStarted());
    EMBER_CHECK(scene.GetAudioSourceCount() == 2);
    EMBER_CHECK(scene.IsAudioSourcePlaying(ambienceEntity.GetId()));
    EMBER_CHECK(!scene.IsAudioSourcePlaying(inactiveEntity.GetId()));
    EMBER_CHECK(!scene.IsAudioSourcePlaying(delayedEntity.GetId()));
    EMBER_CHECK(scene.PlayAudioSource(delayedEntity.GetId()));
    EMBER_CHECK(scene.IsAudioSourcePlaying(delayedEntity.GetId()));
    EMBER_CHECK(scene.StopAudioSource(delayedEntity.GetId()));
    EMBER_CHECK(!scene.IsAudioSourcePlaying(delayedEntity.GetId()));
    EMBER_CHECK(!scene.PlayAudioSource(0));
    EMBER_CHECK(!scene.StopAudioSource(0));
    EMBER_CHECK(!scene.IsAudioSourcePlaying(disabledEntity.GetId()));
    EMBER_CHECK(!scene.HasAudioSource(disabledEntity.GetId()));
    EMBER_CHECK(scene.HasAudioSource(ambienceEntity.GetId()));
    EMBER_CHECK(scene.GetAudioDiagnostics().empty());

    scene.Stop();
    EMBER_CHECK(!scene.IsStarted());
    EMBER_CHECK(scene.GetAudioSourceCount() == 0);
    EMBER_CHECK(!scene.IsAudioSourcePlaying(ambienceEntity.GetId()));

    scene.Start(&audio, &assets);
    EMBER_CHECK(scene.GetAudioSourceCount() == 2);
    scene.Stop();

    ember::Scene missingAssetScene("Missing Audio");
    auto& missing = missingAssetScene.CreateEntity("Missing");
    auto& missingSource = missing.AddComponent<ember::AudioSource2D>();
    missingSource.audioAssetId = "not-found";
    missingAssetScene.Start(&audio, &assets);
    EMBER_CHECK(missingAssetScene.GetAudioSourceCount() == 0);
    EMBER_CHECK(!missingAssetScene.GetAudioDiagnostics().empty());
    missingAssetScene.Stop();

    // A missing AudioEngine and a missing AssetRegistry are reported per active source.
    ember::Scene missingEngineScene("Missing Audio Engine");
    auto& noEngineEntity = missingEngineScene.CreateEntity("No Engine");
    auto& noEngineSource = noEngineEntity.AddComponent<ember::AudioSource2D>();
    noEngineSource.audioAssetId = "audio-ambience";
    ember::Logger audioLogger;
    missingEngineScene.SetLogger(&audioLogger);
    missingEngineScene.Start(nullptr, &assets);
    EMBER_CHECK(missingEngineScene.GetAudioSourceCount() == 0);
    EMBER_CHECK(missingEngineScene.GetAudioDiagnostics().size() == 1);
    const auto audioLogs = audioLogger.Snapshot();
    EMBER_CHECK(audioLogs.size() == 1 && audioLogs.front().category == "Audio" &&
        audioLogs.front().level == ember::LogLevel::Warning);
    missingEngineScene.Update(1.0F / 60.0F);
    EMBER_CHECK(audioLogger.Size() == 1);
    missingEngineScene.Stop();

    ember::Scene missingRegistryScene("Missing Asset Registry");
    auto& noRegistryEntity = missingRegistryScene.CreateEntity("No Registry");
    auto& noRegistrySource = noRegistryEntity.AddComponent<ember::AudioSource2D>();
    noRegistrySource.audioAssetId = "audio-ambience";
    missingRegistryScene.Start(&audio, nullptr);
    EMBER_CHECK(missingRegistryScene.GetAudioSourceCount() == 0);
    EMBER_CHECK(missingRegistryScene.GetAudioDiagnostics().size() == 1);
    missingRegistryScene.Stop();

    ember::AudioEngine unavailableAudio;
    ember::Scene unavailableEngineScene("Unavailable Audio Engine");
    auto& unavailableEntity = unavailableEngineScene.CreateEntity("Unavailable Engine");
    auto& unavailableSource = unavailableEntity.AddComponent<ember::AudioSource2D>();
    unavailableSource.audioAssetId = "audio-ambience";
    unavailableEngineScene.Start(&unavailableAudio, &assets);
    EMBER_CHECK(unavailableEngineScene.GetAudioSourceCount() == 0);
    EMBER_CHECK(unavailableEngineScene.GetAudioDiagnostics().size() == 1);
    unavailableEngineScene.Stop();

    // Wrong-type and traversal paths are rejected before any sound is loaded.
    ember::AssetMetadata textureMetadata;
    textureMetadata.assetId = "texture-not-audio";
    textureMetadata.type = ember::AssetType::Texture;
    textureMetadata.source = "audio/ambience.wav";
    EMBER_CHECK(ember::AssetMetadataStore::Save(root / "audio" / "texture.emberasset", textureMetadata));
    const auto outsideWave = root.parent_path() / "ember_audio_scene_outside.wav";
    WriteTinyWave(outsideWave);
    ember::AssetMetadata traversalMetadata;
    traversalMetadata.assetId = "audio-traversal";
    traversalMetadata.type = ember::AssetType::Audio;
    traversalMetadata.source = "../ember_audio_scene_outside.wav";
    // Write an intentionally invalid sidecar directly: Save() correctly refuses
    // traversal paths, while Scan()/Load() must also reject hand-edited metadata.
    {
        std::ofstream sidecar(root / "audio" / "traversal.emberasset", std::ios::binary);
        sidecar << R"({"schema_version":1,"asset_id":"audio-traversal","type":"audio","source":"../ember_audio_scene_outside.wav"})";
        EMBER_CHECK(sidecar.good());
    }
    EMBER_CHECK(assets.Scan());
    EMBER_CHECK(assets.Find("audio-traversal") == nullptr);

    ember::Scene invalidReferencesScene("Invalid Audio References");
    auto& wrongTypeEntity = invalidReferencesScene.CreateEntity("Wrong Type");
    auto& wrongTypeSource = wrongTypeEntity.AddComponent<ember::AudioSource2D>();
    wrongTypeSource.audioAssetId = "texture-not-audio";
    auto& traversalEntity = invalidReferencesScene.CreateEntity("Traversal Path");
    auto& traversalSource = traversalEntity.AddComponent<ember::AudioSource2D>();
    traversalSource.audioAssetId = "audio-traversal";
    invalidReferencesScene.Start(&audio, &assets);
    EMBER_CHECK(invalidReferencesScene.GetAudioSourceCount() == 0);
    EMBER_CHECK(invalidReferencesScene.GetAudioDiagnostics().size() == 2);
    invalidReferencesScene.Stop();

    ember::Scene invalidConfigurationScene("Invalid Audio Configuration");
    auto& invalidConfigEntity = invalidConfigurationScene.CreateEntity("Invalid Volume");
    auto& invalidConfigSource = invalidConfigEntity.AddComponent<ember::AudioSource2D>();
    invalidConfigSource.audioAssetId = "audio-ambience";
    invalidConfigSource.volume = std::numeric_limits<float>::quiet_NaN();
    invalidConfigurationScene.Start(&audio, &assets);
    EMBER_CHECK(invalidConfigurationScene.GetAudioSourceCount() == 0);
    EMBER_CHECK(invalidConfigurationScene.GetAudioDiagnostics().size() == 1);
    invalidConfigurationScene.Stop();

    // Moving a running Scene transfers ownership of its runtime sound handles.
    ember::Scene moveSource("Move Source");
    auto& movingEntity = moveSource.CreateEntity("Moving Audio");
    auto& movingSource = movingEntity.AddComponent<ember::AudioSource2D>();
    movingSource.audioAssetId = "audio-ambience";
    movingSource.loop = true;
    moveSource.Start(&audio, &assets);
    EMBER_CHECK(moveSource.IsAudioSourcePlaying(movingEntity.GetId()));

    ember::Scene moveTarget("Move Target");
    auto& replacedEntity = moveTarget.CreateEntity("Replaced Audio");
    auto& replacedSource = replacedEntity.AddComponent<ember::AudioSource2D>();
    replacedSource.audioAssetId = "audio-ambience";
    moveTarget.Start(&audio, &assets);
    EMBER_CHECK(moveTarget.IsAudioSourcePlaying(replacedEntity.GetId()));

    moveTarget = std::move(moveSource);
    EMBER_CHECK(!moveSource.IsStarted());
    EMBER_CHECK(moveTarget.IsStarted());
    EMBER_CHECK(moveTarget.GetAudioSourceCount() == 1);
    EMBER_CHECK(moveTarget.IsAudioSourcePlaying(movingEntity.GetId()));
    EMBER_CHECK(!moveTarget.IsAudioSourcePlaying(replacedEntity.GetId()));
    moveTarget.Stop();
    EMBER_CHECK(!moveTarget.IsAudioSourcePlaying(movingEntity.GetId()));

    audio.Shutdown();
    std::filesystem::remove_all(root, error);
    std::filesystem::remove(outsideWave, error);
    return 0;
}
