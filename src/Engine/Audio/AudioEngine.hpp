#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

namespace ember
{
struct AudioSoundHandle
{
    std::uint32_t id = 0;
    [[nodiscard]] explicit operator bool() const noexcept { return id != 0; }
    friend bool operator==(AudioSoundHandle, AudioSoundHandle) = default;
};

struct AudioEngineConfig
{
    // Tests and offline tools can initialize the mixer without opening a device.
    bool noDevice = false;
    std::uint32_t channels = 2;
    std::uint32_t sampleRate = 48000;
};

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;

    [[nodiscard]] bool Initialize(const AudioEngineConfig& config = {});
    void Shutdown() noexcept;
    [[nodiscard]] bool IsInitialized() const noexcept;

    [[nodiscard]] AudioSoundHandle LoadSound(const std::filesystem::path& path, bool streaming = false);
    [[nodiscard]] bool Play(AudioSoundHandle sound) noexcept;
    [[nodiscard]] bool Stop(AudioSoundHandle sound) noexcept;
    [[nodiscard]] bool SetVolume(AudioSoundHandle sound, float volume) noexcept;
    [[nodiscard]] bool SetLooping(AudioSoundHandle sound, bool looping) noexcept;
    [[nodiscard]] bool IsPlaying(AudioSoundHandle sound) const noexcept;
    [[nodiscard]] bool UnloadSound(AudioSoundHandle sound) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace ember
