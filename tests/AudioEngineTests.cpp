#include "Engine/Audio/AudioEngine.hpp"

#include <array>
#include "TestSupport.hpp"
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

namespace
{
void WriteTinyWave(const std::filesystem::path& path)
{
    constexpr std::uint32_t sampleRate = 8000;
    constexpr std::uint16_t channels = 1;
    constexpr std::uint16_t bitsPerSample = 16;
    constexpr std::uint32_t sampleCount = 80;
    constexpr std::uint32_t dataBytes = sampleCount * sizeof(std::int16_t);
    const std::uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    const std::uint16_t blockAlign = channels * bitsPerSample / 8;
    std::ofstream out(path, std::ios::binary);
    auto write = [&out](const auto& value)
    {
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
    for (std::uint32_t i = 0; i < sampleCount; ++i)
    {
        const std::int16_t sample = static_cast<std::int16_t>(
            std::sin(static_cast<double>(i) * 0.2) * 12000.0);
        write(sample);
    }
}
}

int main()
{
    ember::AudioEngine audio;
    EMBER_CHECK(!audio.IsInitialized());
    EMBER_CHECK(!audio.Play({1}));
    EMBER_CHECK(!audio.Initialize({true, 0, 48000}));
    EMBER_CHECK(!audio.Initialize({true, 2, 1000}));
    EMBER_CHECK(audio.Initialize({true, 2, 48000}));
    EMBER_CHECK(audio.Initialize({true, 2, 48000}));

    const auto path = std::filesystem::temp_directory_path() / "ember_audio_engine_test.wav";
    WriteTinyWave(path);
    const auto sound = audio.LoadSound(path);
    EMBER_CHECK(sound);
    EMBER_CHECK(!audio.LoadSound(path.string() + ".missing"));
    EMBER_CHECK(!audio.SetVolume(sound, -0.1F));
    EMBER_CHECK(!audio.SetVolume(sound, std::numeric_limits<float>::quiet_NaN()));
    EMBER_CHECK(!audio.SetVolume(sound, 4.1F));
    EMBER_CHECK(audio.SetVolume(sound, 0.5F));
    EMBER_CHECK(audio.SetLooping(sound, true));
    EMBER_CHECK(audio.Play(sound));
    EMBER_CHECK(audio.IsPlaying(sound));
    EMBER_CHECK(audio.Stop(sound));
    EMBER_CHECK(audio.SetLooping(sound, false));
    EMBER_CHECK(audio.UnloadSound(sound));
    EMBER_CHECK(!audio.Play(sound));
    EMBER_CHECK(!audio.UnloadSound(sound));

    const auto streamedSound = audio.LoadSound(path, true);
    EMBER_CHECK(streamedSound);
    EMBER_CHECK(audio.SetLooping(streamedSound, true));
    EMBER_CHECK(audio.Play(streamedSound));
    EMBER_CHECK(audio.UnloadSound(streamedSound));

    std::error_code error;
    std::filesystem::remove(path, error);
    audio.Shutdown();
    EMBER_CHECK(!audio.IsInitialized());
    EMBER_CHECK(!audio.Play(sound));
    EMBER_CHECK(audio.Initialize({true, 2, 48000}));
    EMBER_CHECK(!audio.Play(sound)); // Handles from before Shutdown must never become valid again.
    audio.Shutdown();
    return 0;
}
