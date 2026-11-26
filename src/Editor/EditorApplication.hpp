#pragma once

#include "Engine/Resources/AssetImportService.hpp"
#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Scene/Scene.hpp"

#include <cstdint>
#include <string>

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
    void RefreshAssets();
    void LoadSelectedTextureSettings();

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_isRunning = false;
    Scene m_scene{"Untitled Scene"};
    Camera2D m_viewportCamera;
    AssetRegistry m_assetRegistry{"assets"};
    AssetImportService m_assetImportService{"assets"};
    EntityId m_selectedEntity = 0;
    std::string m_selectedAssetId;
    std::string m_importSettingsAssetId;
    int m_textureFilter = 0;
    bool m_premultiplyAlpha = false;
    char m_scenePath[260] = "untitled.emberscene";
    char m_assetSearch[128]{};
    int m_assetTypeFilter = 0;
    char m_statusMessage[256] = "Ready";
};
} // namespace ember
