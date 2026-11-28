#include "Engine/Audio/AudioEngine.hpp"
#include "Engine/Core/FileSystem.hpp"

#include <miniaudio.h>

#include <cmath>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>

namespace ember
{
struct AudioEngine::Impl
{
    struct Sound
    {
        ma_sound value{};
    };

    ma_engine engine{};
    bool initialized = false;
    std::uint32_t nextSoundId = 1;
    std::unordered_map<std::uint32_t, std::unique_ptr<Sound>> sounds;
};

AudioEngine::AudioEngine() : m_impl(std::make_unique<Impl>()) {}
AudioEngine::~AudioEngine() { Shutdown(); }
bool AudioEngine::Initialize(const AudioEngineConfig& config)
{
    if (!m_impl) m_impl = std::make_unique<Impl>();
    if (m_impl->initialized) return true;
    if (config.channels == 0 || config.channels > 8 ||
        config.sampleRate < 8000 || config.sampleRate > 192000)
        return false;

    ma_engine_config engineConfig = ma_engine_config_init();
    if (config.noDevice)
    {
        engineConfig.noDevice = MA_TRUE;
        engineConfig.channels = config.channels;
        engineConfig.sampleRate = config.sampleRate;
    }

    if (ma_engine_init(&engineConfig, &m_impl->engine) != MA_SUCCESS)
        return false;

    m_impl->initialized = true;
    return true;
}

void AudioEngine::Shutdown() noexcept
{
    if (!m_impl || !m_impl->initialized) return;
    for (auto& [id, sound] : m_impl->sounds)
    {
        (void)id;
        ma_sound_uninit(&sound->value);
    }
    m_impl->sounds.clear();
    ma_engine_uninit(&m_impl->engine);
    m_impl->initialized = false;
}

bool AudioEngine::IsInitialized() const noexcept
{
    return m_impl && m_impl->initialized;
}

AudioSoundHandle AudioEngine::LoadSound(const std::filesystem::path& path, bool streaming)
{
    if (!IsInitialized() || path.empty() || m_impl->nextSoundId == 0)
        return {};

    auto sound = std::make_unique<Impl::Sound>();
    const ma_uint32 flags = streaming ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
    const std::string utf8Path = PathToUtf8(path);
    if (ma_sound_init_from_file(&m_impl->engine, utf8Path.c_str(), flags,
            nullptr, nullptr, &sound->value) != MA_SUCCESS)
        return {};

    const std::uint32_t id = m_impl->nextSoundId++;
    m_impl->sounds.emplace(id, std::move(sound));
    return {id};
}

bool AudioEngine::Play(AudioSoundHandle sound) noexcept
{
    if (!IsInitialized() || !sound) return false;
    const auto it = m_impl->sounds.find(sound.id);
    return it != m_impl->sounds.end() &&
        ma_sound_start(&it->second->value) == MA_SUCCESS;
}

bool AudioEngine::SeekToStart(AudioSoundHandle sound) noexcept
{
    if (!IsInitialized() || !sound) return false;
    const auto it = m_impl->sounds.find(sound.id);
    return it != m_impl->sounds.end() &&
        ma_sound_seek_to_pcm_frame(&it->second->value, 0) == MA_SUCCESS;
}

bool AudioEngine::Stop(AudioSoundHandle sound) noexcept
{
    if (!IsInitialized() || !sound) return false;
    const auto it = m_impl->sounds.find(sound.id);
    return it != m_impl->sounds.end() &&
        ma_sound_stop(&it->second->value) == MA_SUCCESS;
}

bool AudioEngine::SetVolume(AudioSoundHandle sound, float volume) noexcept
{
    if (!IsInitialized() || !sound || !std::isfinite(volume) ||
        volume < 0.0F || volume > 4.0F)
        return false;
    const auto it = m_impl->sounds.find(sound.id);
    if (it == m_impl->sounds.end()) return false;
    ma_sound_set_volume(&it->second->value, volume);
    return true;
}

bool AudioEngine::SetLooping(AudioSoundHandle sound, bool looping) noexcept
{
    if (!IsInitialized() || !sound) return false;
    const auto it = m_impl->sounds.find(sound.id);
    if (it == m_impl->sounds.end()) return false;
    ma_sound_set_looping(&it->second->value, looping ? MA_TRUE : MA_FALSE);
    return true;
}

bool AudioEngine::IsPlaying(AudioSoundHandle sound) const noexcept
{
    if (!IsInitialized() || !sound) return false;
    const auto it = m_impl->sounds.find(sound.id);
    return it != m_impl->sounds.end() &&
        ma_sound_is_playing(&it->second->value) == MA_TRUE;
}

bool AudioEngine::UnloadSound(AudioSoundHandle sound) noexcept
{
    if (!IsInitialized() || !sound) return false;
    const auto it = m_impl->sounds.find(sound.id);
    if (it == m_impl->sounds.end()) return false;
    ma_sound_uninit(&it->second->value);
    m_impl->sounds.erase(it);
    return true;
}
} // namespace ember
