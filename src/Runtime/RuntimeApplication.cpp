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
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(m_window)))
                m_running = false;
        }

        const auto now = Clock::now();
        const float deltaSeconds = std::clamp(
            std::chrono::duration<float>(now - previous).count(), 0.0F, 0.05F);
        previous = now;
        Update(deltaSeconds);
        Render();
    }
    return 0;
}

void RuntimeApplication::Update(float deltaSeconds)
{
    m_inputMap.Update();
    m_scene.Update(deltaSeconds, m_inputMap.GetActionState());
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
            m_renderer.DrawTexture(texture->second, entity.GetTransform(), m_camera,
                visual.size, visual.red, visual.green, visual.blue, visual.alpha);
        }
        else if (visual.textureAssetId.empty())
        {
            m_renderer.DrawFilledRect(entity.GetTransform(), m_camera, visual.size,
                visual.red, visual.green, visual.blue, visual.alpha);
        }
    });
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
