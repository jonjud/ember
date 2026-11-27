#pragma once

namespace ember
{
class Time
{
public:
    void Reset();
    void Tick();

    [[nodiscard]] float DeltaSeconds() const;

private:
    double m_frequency = 0.0;
    unsigned long long m_lastCounter = 0;
    float m_deltaSeconds = 0.0F;
};
} // namespace ember
