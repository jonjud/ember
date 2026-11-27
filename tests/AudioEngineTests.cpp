#include "Engine/Audio/AudioEngine.hpp"

#include <array>
#include <cassert>
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
    assert(!audio.IsInitialized());
    assert(!audio.Play({1}));
    assert(!audio.Initialize({true, 0, 48000}));
    assert(!audio.Initialize({true, 2, 1000}));
    assert(audio.Initialize({true, 2, 48000}));
    assert(audio.Initialize({true, 2, 48000}));

    const auto path = std::filesystem::temp_directory_path() / "ember_audio_engine_test.wav";
    WriteTinyWave(path);
    const auto sound = audio.LoadSound(path);
    assert(sound);
    assert(!audio.LoadSound(path.string() + ".missing"));
    assert(!audio.SetVolume(sound, -0.1F));
    assert(!audio.SetVolume(sound, std::numeric_limits<float>::quiet_NaN()));
    assert(!audio.SetVolume(sound, 4.1F));
    assert(audio.SetVolume(sound, 0.5F));
    assert(audio.SetLooping(sound, true));
    assert(audio.Play(sound));
    assert(audio.IsPlaying(sound));
    assert(audio.Stop(sound));
    assert(audio.SetLooping(sound, false));
    assert(audio.UnloadSound(sound));
    assert(!audio.Play(sound));
    assert(!audio.UnloadSound(sound));

    std::error_code error;
    std::filesystem::remove(path, error);
    audio.Shutdown();
    assert(!audio.IsInitialized());
    assert(!audio.Play(sound));
    assert(audio.Initialize({true, 2, 48000}));
    audio.Shutdown();
    return 0;
}
