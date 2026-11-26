#pragma once

#include "Engine/Core/Time.hpp"
#include "Engine/Resources/AssetImportService.hpp"
#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Platform/InputMap.hpp"
#include "Engine/Project/ProjectConfiguration.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;
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
    void UpdatePlayMode();
    void DrawMenuBar();
    void DrawSceneHierarchy();
    void DrawInspector();
    void DrawAssetBrowser();
    void DrawInputSettings();
    void DrawProjectSettings();
    void DrawViewport();
    void CreateEntity();
    void StartPlayMode();
    void StopPlayMode();
    [[nodiscard]] Scene& GetActiveScene();
    void SaveScene();
    void LoadScene();
    void RefreshAssets();
    void LoadSelectedTextureSettings();
    void LoadInputSettings();
    [[nodiscard]] bool SaveInputSettings();
    [[nodiscard]] bool SaveProjectSettings();
    [[nodiscard]] SDL_Texture* GetPreviewTexture(const AssetMetadata& metadata);
    void ClearPreviewTextures();

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<std::string, SDL_Texture*> m_previewTextures;
    bool m_isRunning = false;
    Scene m_scene{"Untitled Scene"};
    Scene m_playScene{"Play Scene"};
    bool m_isPlaying = false;
    Time m_time;
    InputMap m_inputMap = InputMap::CreateDefaultMovementMap();
    ProjectConfiguration m_projectConfiguration;
    std::filesystem::path m_projectFilePath{"ember.emberproject"};
    std::vector<std::string> m_customInputActions;
    std::string m_rebindAction;
    bool m_replaceActionOnRebind = true;
    char m_projectName[128] = "Untitled Project";
    char m_startupScenePath[260]{};
    char m_newInputActionName[64]{};
    char m_renamedInputActionName[64]{};
    std::string m_selectedCustomInputAction;
    Camera2D m_viewportCamera;
    AssetRegistry m_assetRegistry{"assets"};
    AssetImportService m_assetImportService{"assets"};
    EntityId m_selectedEntity = 0;
    EntityId m_dragEntityId = 0;
    Vector2 m_dragStartMouseWorld{};
    Vector2 m_dragStartEntityPosition{};
    Vector2 m_dragGizmoAxis{};
    bool m_draggingGizmoAxis = false;
    int m_gizmoMode = 0;
    bool m_gizmoLocalSpace = false;
    int m_dragGizmoAxisIndex = 0;
    bool m_snapTransforms = false;
    float m_positionSnap = 16.0F;
    float m_rotationSnapDegrees = 15.0F;
    float m_scaleSnap = 0.1F;
    bool m_draggingRotationGizmo = false;
    bool m_draggingScaleGizmo = false;
    float m_dragStartEntityRotation = 0.0F;
    float m_dragStartMouseAngle = 0.0F;
    Vector2 m_dragStartEntityScale{1.0F, 1.0F};
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
