#pragma once

#include "Engine/Scene/Scene.hpp"

#include <cstdint>

struct SDL_Renderer;
struct SDL_Window;

namespace ember
{
class EditorApplication
{
public:
    EditorApplication() = default;
    ~EditorApplication();

    EditorApplication(const EditorApplication&) = delete;
    EditorApplication& operator=(const EditorApplication&) = delete;

    [[nodiscard]] bool Initialize();
    void Run();
    void Shutdown();

private:
    void ProcessEvents();
    void Render();
    void DrawMenuBar();
    void DrawSceneHierarchy();
    void DrawInspector();
    void DrawAssetBrowser();
    void DrawViewport();
    void CreateEntity();
    void SaveScene();
    void LoadScene();

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_isRunning = false;
    Scene m_scene{"Untitled Scene"};
    EntityId m_selectedEntity = 0;
    char m_scenePath[260] = "untitled.emberscene";
    char m_statusMessage[256] = "Ready";
};
} // namespace ember
