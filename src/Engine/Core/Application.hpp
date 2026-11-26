#pragma once

#include "Engine/Core/Time.hpp"

#include <SDL3/SDL.h>

namespace ember
{
class Application
{
public:
    Application() = default;
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    [[nodiscard]] bool Initialize();
    void Run();
    void Shutdown();

private:
    void ProcessEvents();
    void Update();
    void Render();

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_isRunning = false;
    float m_playerX = 0.0F;
    float m_playerY = 0.0F;
    Time m_time;
};
} // namespace ember
