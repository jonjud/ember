#pragma once

#include "Engine/Scene/Component.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>

namespace ember
{
class SpriteAnimation2D final : public Component
{
public:
    int frameWidth = 32;
    int frameHeight = 32;
    int columns = 1;
    int frameCount = 1;
    float framesPerSecond = 8.0F;
    bool loop = true;
    bool playing = true;

    [[nodiscard]] bool IsConfigurationValid() const noexcept
    {
        return frameWidth > 0 && frameHeight > 0 && columns > 0 &&
            frameCount > 0 && frameCount <= 100000 && columns <= 100000 &&
            frameWidth <= 65536 && frameHeight <= 65536 &&
            std::isfinite(framesPerSecond) && framesPerSecond > 0.0F &&
            framesPerSecond <= 1000.0F;
    }

    [[nodiscard]] int GetCurrentFrame() const noexcept { return m_currentFrame; }

    void Play() noexcept
    {
        if (IsConfigurationValid())
        {
            if (!playing && !loop && m_currentFrame >= frameCount - 1)
                ResetPlayback();
            playing = true;
        }
    }
    void Pause() noexcept { playing = false; }
    void Stop() noexcept { playing = false; ResetPlayback(); }

    void OnCreate(Entity& entity) noexcept override
    {
        (void)entity;
        ResetPlayback();
    }

    void OnUpdate(Entity& entity, float deltaSeconds) noexcept override
    {
        (void)entity;
        if (!playing || !IsConfigurationValid() ||
            !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F)
            return;

        if (m_currentFrame >= frameCount) m_currentFrame = 0;
        m_elapsedSeconds += std::min(static_cast<double>(deltaSeconds), 0.25);
        const double frameDuration = 1.0 / static_cast<double>(framesPerSecond);
        const auto framesToAdvance = static_cast<std::size_t>(m_elapsedSeconds / frameDuration);
        if (framesToAdvance == 0) return;
        m_elapsedSeconds -= static_cast<double>(framesToAdvance) * frameDuration;

        if (loop)
        {
            m_currentFrame = static_cast<int>(
                (static_cast<std::size_t>(m_currentFrame) + framesToAdvance) %
                static_cast<std::size_t>(frameCount));
        }
        else
        {
            const std::size_t remaining = static_cast<std::size_t>(frameCount - 1 - m_currentFrame);
            if (framesToAdvance >= remaining)
            {
                m_currentFrame = frameCount - 1;
                playing = false;
                m_elapsedSeconds = 0.0;
            }
            else
            {
                m_currentFrame += static_cast<int>(framesToAdvance);
            }
        }
    }

    [[nodiscard]] std::unique_ptr<Component> Clone() const override
    {
        auto copy = std::make_unique<SpriteAnimation2D>(*this);
        copy->ResetPlayback();
        return copy;
    }

private:
    void ResetPlayback() noexcept
    {
        m_currentFrame = 0;
        m_elapsedSeconds = 0.0;
    }
    int m_currentFrame = 0;
    double m_elapsedSeconds = 0.0;
};
} // namespace ember
