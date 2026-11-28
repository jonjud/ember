#include "Runtime/RuntimeApplication.hpp"

#include "Engine/Core/FileSystem.hpp"
#include "Engine/Physics/PhysicsComponentRegistration.hpp"
#include "Engine/Project/ProjectConfigurationStore.hpp"
#include "Engine/Scene/AnimationComponentRegistration.hpp"
#include "Engine/Scene/AudioComponentRegistration.hpp"
#include "Engine/Scene/SceneSerializer.hpp"
#include "Engine/Scripting/ScriptComponentRegistration.hpp"
#include "Engine/Scripting/LuaScriptRuntime.hpp"
#include "Engine/UI/UIComponentRegistration.hpp"
#include "Engine/UI/UIComponents2D.hpp"
#include "Engine/UI/UILayout2D.hpp"
#include "Engine/Scene/AnimationComponents2D.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

namespace ember
{
RuntimeApplication::~RuntimeApplication()
{
    Shutdown();
}

bool RuntimeApplication::Initialize(const std::filesystem::path& projectFile)
{
    m_logger.AddSink(std::make_shared<ConsoleLogSink>());
    if (!LoadProject(projectFile))
        return false;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        m_logger.Error("Runtime", std::string("SDL initialization failed: ") + SDL_GetError());
        return false;
    }

    m_window = SDL_CreateWindow(m_projectConfiguration.name.c_str(), 1280, 720,
        SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr)
    {
        m_logger.Error("Runtime", std::string("Window creation failed: ") + SDL_GetError());
        Shutdown();
        return false;
    }

    if (!m_renderer.Initialize(m_window))
    {
        Shutdown();
        return false;
    }

    if (!m_audio.Initialize())
        m_logger.Warning("Audio", "Audio device unavailable; continuing without audio");

    m_camera.SetPosition({0.0F, 0.0F});
    m_camera.SetZoom(1.0F);

    if (!LoadStartupScene())
    {
        Shutdown();
        return false;
    }

    m_scene.SetLogger(&m_logger);
    m_scriptRuntime = std::make_unique<LuaScriptRuntime>(m_assetRegistry);
    m_scene.Start(&m_audio, &m_assetRegistry, m_scriptRuntime.get());
    m_initialized = true;
    m_running = true;
    m_logger.Info("Runtime", "Game runtime initialized");
    return true;
}

bool RuntimeApplication::LoadProject(const std::filesystem::path& projectFile)
{
    std::string error;
    const auto absoluteProject = std::filesystem::absolute(projectFile).lexically_normal();
    const auto configuration = ProjectConfigurationStore::Load(absoluteProject, &error);
    if (!configuration)
    {
        m_logger.Error("Project", "Could not load project '" +
            PathToUtf8(absoluteProject) + "': " + error);
        return false;
    }

    m_projectFilePath = absoluteProject;
    m_projectConfiguration = *configuration;
    const auto assetRoot = ProjectConfigurationStore::ResolveAssetDirectory(
        m_projectFilePath, m_projectConfiguration);
    if (!assetRoot)
    {
        m_logger.Error("Project", "Project asset directory is not a safe relative path");
        return false;
    }
    m_assetRegistry = AssetRegistry(*assetRoot);
    if (!m_assetRegistry.Scan())
    {
        m_logger.Error("Assets", "Could not scan the project asset directory");
        return false;
    }
    for (const auto& diagnostic : m_assetRegistry.GetDiagnostics())
        m_logger.Warning("Assets", diagnostic.message);

    if (!RegisterPhysicsComponents(m_componentRegistry) ||
        !RegisterAnimationComponents(m_componentRegistry) ||
        !RegisterAudioComponents(m_componentRegistry) ||
        !RegisterScriptComponent(m_componentRegistry) ||
        !RegisterUIComponents(m_componentRegistry))
    {
        m_logger.Error("Runtime", "Could not register built-in runtime components");
        return false;
    }

    if (m_projectConfiguration.inputBindingsConfigured)
    {
        for (const auto& [action, keys] : m_projectConfiguration.inputBindings)
        {
            (void)m_inputMap.CreateAction(action);
            for (const auto key : keys)
            {
                if (!m_inputMap.BindAction(action, static_cast<SDL_Scancode>(key)))
                    m_logger.Warning("Input", "Ignored invalid input binding for action: " + action);
            }
        }
    }
    else
    {
        m_inputMap = InputMap::CreateDefaultMovementMap();
    }
    return true;
}

bool RuntimeApplication::LoadStartupScene()
{
    const auto scenePath = ProjectConfigurationStore::ResolveStartupScene(
        m_projectFilePath, m_projectConfiguration);
    if (!scenePath || m_projectConfiguration.startupScene.empty())
    {
        m_logger.Error("Project", "Project does not define a valid startup scene");
        return false;
    }

    std::string error;
    if (!SceneSerializer::Load(m_scene, *scenePath, &error, &m_componentRegistry))
    {
        m_logger.Error("Scene", "Could not load startup scene: " + error);
        return false;
    }

    for (const auto& [assetId, metadata] : m_assetRegistry.GetAssets())
    {
        if (metadata.type != AssetType::Texture)
            continue;
        const auto path = m_assetRegistry.GetDirectory().Resolve(metadata.source);
        if (!path)
            continue;
        const TextureHandle texture = m_renderer.LoadTexture(
            PathToUtf8(*path), metadata.texture);
        if (texture)
            m_textures.emplace(assetId, texture);
        else
            m_logger.Warning("Assets", "Could not load texture asset: " + assetId);
    }
    return true;
}

int RuntimeApplication::Run()
{
    if (!m_initialized)
        return 1;

    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    while (m_running)
    {
        bool pointerPressed = false;
        bool pointerReleased = false;
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(m_window)))
                m_running = false;
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                event.button.button == SDL_BUTTON_LEFT)
                pointerPressed = true;
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                event.button.button == SDL_BUTTON_LEFT)
                pointerReleased = true;
        }

        const auto now = Clock::now();
        const float deltaSeconds = std::clamp(
            std::chrono::duration<float>(now - previous).count(), 0.0F, 0.05F);
        previous = now;
        Update(deltaSeconds, pointerPressed, pointerReleased);
        Render();
    }
    return 0;
}

void RuntimeApplication::Update(float deltaSeconds, bool pointerPressed, bool pointerReleased)
{
    m_inputMap.Update();
    m_scene.Update(deltaSeconds, m_inputMap.GetActionState());

    float mouseX = 0.0F;
    float mouseY = 0.0F;
    const SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
    int width = 0;
    int height = 0;
    (void)SDL_GetWindowSize(m_window, &width, &height);

    const UICanvas2D* canvas = nullptr;
    m_scene.ForEachEntity([&canvas](const Entity& entity)
    {
        if (canvas == nullptr && entity.IsActive())
        {
            const auto* candidate = entity.GetComponent<UICanvas2D>();
            if (candidate != nullptr && candidate->enabled && candidate->IsConfigurationValid())
                canvas = candidate;
        }
    });
    m_uiInputRouter.Update(m_scene, canvas,
        {static_cast<float>(width), static_cast<float>(height)},
        {mouseX, mouseY}, (mouseButtons & SDL_BUTTON_LMASK) != 0,
        pointerPressed, pointerReleased);
    for (const auto& click : m_uiInputRouter.GetEvents())
        (void)m_scene.DispatchUIButtonClicked(click.entity);
}

void RuntimeApplication::Render()
{
    int width = 0;
    int height = 0;
    (void)SDL_GetWindowSize(m_window, &width, &height);
    m_camera.SetViewportSize(static_cast<float>(width), static_cast<float>(height));

    m_renderer.Clear(24, 27, 35);
    m_scene.ForEachEntity([this](const Entity& entity)
    {
        if (!entity.IsActive())
            return;
        const auto& visual = entity.GetRenderComponent();
        if (!visual.visible)
            return;

        const auto texture = m_textures.find(visual.textureAssetId);
        if (!visual.textureAssetId.empty() && texture != m_textures.end())
        {
            const auto* animation = entity.GetComponent<SpriteAnimation2D>();
            if (animation != nullptr && animation->IsConfigurationValid())
                m_renderer.DrawTexture(texture->second, entity.GetTransform(), m_camera,
                    visual.size, animation->GetSourcePosition(), animation->GetSourceSize(),
                    visual.red, visual.green, visual.blue, visual.alpha);
            else
                m_renderer.DrawTexture(texture->second, entity.GetTransform(), m_camera,
                    visual.size, visual.red, visual.green, visual.blue, visual.alpha);
        }
        else if (visual.textureAssetId.empty())
        {
            m_renderer.DrawFilledRect(entity.GetTransform(), m_camera, visual.size,
                visual.red, visual.green, visual.blue, visual.alpha);
        }
    });

    const Vector2 viewport{static_cast<float>(width), static_cast<float>(height)};
    const UICanvas2D* canvas = nullptr;
    m_scene.ForEachEntity([&canvas](const Entity& entity)
    {
        if (canvas == nullptr && entity.IsActive())
        {
            const auto* candidate = entity.GetComponent<UICanvas2D>();
            if (candidate != nullptr && candidate->enabled && candidate->IsConfigurationValid())
                canvas = candidate;
        }
    });
    UICanvasLayout2D layout;
    if (canvas != nullptr && ResolveUICanvasLayout(*canvas, viewport, layout))
    {
        m_scene.ForEachEntity([this, &layout](const Entity& entity)
        {
            if (!entity.IsActive()) return;
            if (const auto* panel = entity.GetComponent<UIPanel2D>())
            {
                if (panel->enabled && panel->IsConfigurationValid())
                {
                    const Vector2 position = layout.origin + panel->position * layout.scale;
                    const Vector2 size = panel->size * layout.scale;
                    m_renderer.DrawFilledRect(position.x, position.y, size.x, size.y,
                        static_cast<std::uint8_t>(panel->red * 255.0F),
                        static_cast<std::uint8_t>(panel->green * 255.0F),
                        static_cast<std::uint8_t>(panel->blue * 255.0F),
                        static_cast<std::uint8_t>(panel->alpha * 255.0F));
                }
            }
            if (const auto* button = entity.GetComponent<UIButton2D>())
            {
                if (button->enabled && button->IsConfigurationValid())
                {
                    const Vector2 position = layout.origin + button->position * layout.scale;
                    const Vector2 size = button->size * layout.scale;
                    const auto& state = m_uiInputRouter.GetState(entity.GetId());
                    const float boost = state.hovered ? 1.18F : 1.0F;
                    m_renderer.DrawFilledRect(position.x, position.y, size.x, size.y,
                        static_cast<std::uint8_t>(std::min(1.0F, button->red * boost) * 255.0F),
                        static_cast<std::uint8_t>(std::min(1.0F, button->green * boost) * 255.0F),
                        static_cast<std::uint8_t>(std::min(1.0F, button->blue * boost) * 255.0F),
                        static_cast<std::uint8_t>(button->alpha * 255.0F));
                    m_renderer.DrawDebugText(position.x + 8.0F * layout.scale,
                        position.y + 8.0F * layout.scale, button->text);
                }
            }
            if (const auto* text = entity.GetComponent<UIText2D>())
            {
                if (text->enabled && text->IsConfigurationValid())
                {
                    const Vector2 position = layout.origin + text->position * layout.scale;
                    m_renderer.DrawDebugText(position.x, position.y, text->text,
                        static_cast<std::uint8_t>(text->red * 255.0F),
                        static_cast<std::uint8_t>(text->green * 255.0F),
                        static_cast<std::uint8_t>(text->blue * 255.0F),
                        static_cast<std::uint8_t>(text->alpha * 255.0F));
                }
            }
        });
    }
    m_renderer.Present();
}

void RuntimeApplication::Shutdown() noexcept
{
    if (m_scene.IsStarted())
        m_scene.Stop();
    m_scriptRuntime.reset();
    for (const auto& [assetId, texture] : m_textures)
    {
        (void)assetId;
        m_renderer.DestroyTexture(texture);
    }
    m_textures.clear();
    m_renderer.Shutdown();
    m_audio.Shutdown();
    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
    m_initialized = false;
    m_running = false;
}
} // namespace ember
