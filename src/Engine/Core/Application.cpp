#include "Engine/Core/Application.hpp"

#include "Engine/Platform/Input.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <iostream>

namespace
{
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr float kPlayerSize = 64.0F;
constexpr float kPlayerSpeed = 320.0F;
constexpr float kMaxDeltaSeconds = 0.1F;
}

namespace ember
{
Application::~Application()
{
    Shutdown();
}

bool Application::Initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "EMBER: SDL initialization failed: " << SDL_GetError() << '\n';
        return false;
    }

    if (!m_audio.Initialize())
        std::cerr << "EMBER: Audio device unavailable; continuing without audio.\\n";

    m_window = SDL_CreateWindow(
        "EMBER Engine",
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_RESIZABLE);

    if (m_window == nullptr)
    {
        std::cerr << "EMBER: Window creation failed: " << SDL_GetError() << '\n';
        Shutdown();
        return false;
    }

    if (!m_renderer.Initialize(m_window))
    {
        Shutdown();
        return false;
    }

    if (!m_assets.SetTextureBackend(
            [this](const std::string& path, const TextureImportSettings& settings) { return m_renderer.LoadTexture(path, settings); },
            [this](TextureHandle texture) { m_renderer.DestroyTexture(texture); }))
    {
        std::cerr << "EMBER: Texture cache backend could not be initialized.\n";
        Shutdown();
        return false;
    }

    Entity& player = m_scene.CreateEntity("Player");
    m_playerEntityId = player.GetId();
    player.GetTransform().position = {
        (static_cast<float>(kWindowWidth) - kPlayerSize) * 0.5F,
        (static_cast<float>(kWindowHeight) - kPlayerSize) * 0.5F
    };

    RenderComponent2D& playerVisual = player.GetRenderComponent();
    playerVisual.size = {kPlayerSize, kPlayerSize};
    playerVisual.red = 91;
    playerVisual.green = 160;
    playerVisual.blue = 255;
    playerVisual.visible = true;

    m_isRunning = true;
    m_time.Reset();

    std::cout << "EMBER Engine 0.1.0 started. Use WASD or arrow keys to move; Escape to quit.\n";
    return true;
}

void Application::Run()
{
    while (m_isRunning)
    {
        ProcessEvents();
        m_time.Tick();
        Update();
        Render();
    }
}

void Application::Shutdown()
{
    m_audio.Shutdown();
    m_assets.Clear();
    m_renderer.Shutdown();

    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    m_scene.Clear();
    m_playerEntityId = 0;

    SDL_Quit();
    m_isRunning = false;
}

void Application::ProcessEvents()
{
    SDL_Event event{};

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            m_isRunning = false;
        }
    }
}

void Application::Update()
{
    if (Input::IsKeyDown(SDL_SCANCODE_ESCAPE))
    {
        m_isRunning = false;
    }

    Entity* player = m_scene.FindEntity(m_playerEntityId);
    if (player == nullptr)
    {
        return;
    }

    const float deltaSeconds = std::min(m_time.DeltaSeconds(), kMaxDeltaSeconds);
    float directionX = 0.0F;
    float directionY = 0.0F;

    if (m_inputMap.IsActionDown("MoveLeft"))
    {
        directionX -= 1.0F;
    }
    if (m_inputMap.IsActionDown("MoveRight"))
    {
        directionX += 1.0F;
    }
    if (m_inputMap.IsActionDown("MoveUp"))
    {
        directionY -= 1.0F;
    }
    if (m_inputMap.IsActionDown("MoveDown"))
    {
        directionY += 1.0F;
    }

    // Normalize diagonal movement so it is not faster than horizontal movement.
    if (directionX != 0.0F && directionY != 0.0F)
    {
        constexpr float kDiagonalScale = 0.70710678F;
        directionX *= kDiagonalScale;
        directionY *= kDiagonalScale;
    }

    Transform2D& transform = player->GetTransform();
    transform.position.x += directionX * kPlayerSpeed * deltaSeconds;
    transform.position.y += directionY * kPlayerSpeed * deltaSeconds;

    const RenderComponent2D& visual = player->GetRenderComponent();
    const float renderedWidth = visual.size.x * transform.scale.x;
    const float renderedHeight = visual.size.y * transform.scale.y;

    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);

    transform.position.x = std::clamp(
        transform.position.x,
        0.0F,
        std::max(0.0F, static_cast<float>(windowWidth) - renderedWidth));
    transform.position.y = std::clamp(
        transform.position.y,
        0.0F,
        std::max(0.0F, static_cast<float>(windowHeight) - renderedHeight));
}

void Application::Render()
{
    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);

    m_camera.SetViewportSize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
    m_camera.SetPosition({static_cast<float>(windowWidth) * 0.5F, static_cast<float>(windowHeight) * 0.5F});

    m_renderer.Clear(24, 27, 36);
    m_scene.ForEachEntity([this](const Entity& entity)
    {
        if (!entity.IsActive())
        {
            return;
        }

        const RenderComponent2D& visual = entity.GetRenderComponent();
        if (!visual.visible)
        {
            return;
        }

        if (visual.texture)
        {
            m_renderer.DrawTexture(
                visual.texture,
                entity.GetTransform(),
                m_camera,
                visual.size,
                visual.red,
                visual.green,
                visual.blue,
                visual.alpha);
        }
        else
        {
            m_renderer.DrawFilledRect(
                entity.GetTransform(),
                m_camera,
                visual.size,
                visual.red,
                visual.green,
                visual.blue,
                visual.alpha);
        }
    });
    m_renderer.Present();
}
} // namespace ember
