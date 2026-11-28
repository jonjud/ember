#pragma once

#include "Engine/Audio/AudioEngine.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Platform/InputMap.hpp"
#include "Engine/Project/ProjectConfiguration.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/Renderer2D.hpp"
#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Scene/ComponentRegistry.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scripting/ScriptRuntime.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

struct SDL_Window;

namespace ember
{
class RuntimeApplication
{
public:
    RuntimeApplication() = default;
    ~RuntimeApplication();

    RuntimeApplication(const RuntimeApplication&) = delete;
    RuntimeApplication& operator=(const RuntimeApplication&) = delete;

    [[nodiscard]] bool Initialize(const std::filesystem::path& projectFile);
    int Run();
    void Shutdown() noexcept;

private:
    [[nodiscard]] bool LoadProject(const std::filesystem::path& projectFile);
    [[nodiscard]] bool LoadStartupScene();
    void Update(float deltaSeconds);
    void Render();

    SDL_Window* m_window = nullptr;
    Renderer2D m_renderer;
    AudioEngine m_audio;
    Logger m_logger{2048};
    Scene m_scene{"Game Runtime"};
    ComponentRegistry m_componentRegistry;
    ProjectConfiguration m_projectConfiguration;
    std::filesystem::path m_projectFilePath;
    AssetRegistry m_assetRegistry{"assets"};
    InputMap m_inputMap;
    std::unique_ptr<ScriptRuntime> m_scriptRuntime;
    Camera2D m_camera;
    std::unordered_map<std::string, TextureHandle> m_textures;
    bool m_initialized = false;
    bool m_running = false;
};
} // namespace ember
