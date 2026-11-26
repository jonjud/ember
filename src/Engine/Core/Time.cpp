#include "Engine/Core/Time.hpp"

#include <SDL3/SDL.h>

namespace ember
{
void Time::Reset()
{
    m_frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    m_lastCounter = SDL_GetPerformanceCounter();
    m_deltaSeconds = 0.0F;
}

void Time::Tick()
{
    const unsigned long long currentCounter = SDL_GetPerformanceCounter();

    if (m_frequency <= 0.0)
    {
        m_deltaSeconds = 0.0F;
        m_lastCounter = currentCounter;
        return;
    }

    const double elapsed = static_cast<double>(currentCounter - m_lastCounter) / m_frequency;
    m_deltaSeconds = static_cast<float>(elapsed);
    m_lastCounter = currentCounter;
}

float Time::DeltaSeconds() const
{
    return m_deltaSeconds;
}
} // namespace ember
