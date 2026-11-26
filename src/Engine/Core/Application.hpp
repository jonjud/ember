#pragma once

#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/Renderer2D.hpp"

struct SDL_Window;

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
    Renderer2D m_renderer;
    Camera2D m_camera;
    bool m_isRunning = false;
    float m_playerX = 0.0F;
    float m_playerY = 0.0F;
    Time m_time;
};
} // namespace ember
