#pragma once

#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/Resources/AssetManager.hpp"
#include "Engine/Scene/Scene.hpp"

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
    AssetManager m_assets{"assets"};
    Camera2D m_camera;
    Scene m_scene{"Main Scene"};
    EntityId m_playerEntityId = 0;
    bool m_isRunning = false;
    Time m_time;
};
} // namespace ember
