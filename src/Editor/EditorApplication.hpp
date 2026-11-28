#pragma once

#include "Engine/Core/Time.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Audio/AudioEngine.hpp"
#include "Engine/Resources/AssetImportService.hpp"
#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/ComponentRegistry.hpp"
#include "Engine/Platform/InputMap.hpp"
#include "Engine/UI/UIInputRouter2D.hpp"
#include "Engine/Project/ProjectConfiguration.hpp"
#include "Engine/Project/ProjectExporter.hpp"
#include "Engine/Scripting/ScriptRuntime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
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
    void DrawExportWindowsModal();
    void DrawSceneHierarchy();
    void DrawInspector();
    void DrawAssetBrowser();
    void DrawInputSettings();
    void DrawProjectSettings();
    void DrawScriptDiagnostics();
    void DrawLogWindow();
    void DrawScriptSourceEditor();
    void OpenSelectedScriptSource();
    void SaveScriptSource();
    void ReloadScriptSourceFromDisk();
    void ValidateScriptSource();
    void DrawViewport();
    void DrawUnsavedChangesModal();
    void DrawUnsavedScriptExitModal();
    void RequestExit();
    void CreateEntity();
    void DuplicateSelectedEntity();
    void DeleteSelectedEntity();
    void RequestNewScene(const std::filesystem::path& path);
    void RequestLoadScene(const std::filesystem::path& path);
    void ApplyPendingSceneChange();
    [[nodiscard]] bool LoadSceneFromPath(const std::filesystem::path& path);
    void MarkSceneDirty();
    void StartPlayMode();
    void StopPlayMode();
    [[nodiscard]] Scene& GetActiveScene();
    [[nodiscard]] bool SaveScene();
    void LoadScene();
    void RefreshAssets();
    void LoadSelectedTextureSettings();
    void LoadInputSettings();
    void LoadStartupScene();
    [[nodiscard]] bool SaveInputSettings();
    [[nodiscard]] bool SaveProjectSettings();
    [[nodiscard]] SDL_Texture* GetPreviewTexture(const AssetMetadata& metadata);
    void ClearPreviewTextures();
    void StopAudioPreview();

    SDL_Window* m_window = nullptr;
    AudioEngine m_audio;
    Logger m_logger{2048};
    Logger m_runtimeLogger{2048};
    char m_logTextFilter[128]{};
    int m_logLevelFilter = 0;
    AudioSoundHandle m_audioPreviewSound{};
    std::string m_audioPreviewAssetId;
    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<std::string, SDL_Texture*> m_previewTextures;
    bool m_isRunning = false;
    bool m_pendingScriptExit = false;
    Scene m_scene{"Untitled Scene"};
    ComponentRegistry m_componentRegistry;
    Scene m_playScene{"Play Scene"};
    std::unique_ptr<ScriptRuntime> m_scriptRuntime;
    UIInputRouter2D m_uiInputRouter;
    bool m_isPlaying = false;
    bool m_sceneDirty = false;
    int m_pendingSceneChange = 0;
    std::filesystem::path m_pendingScenePath;
    std::filesystem::path m_currentScenePath{"assets/scenes/untitled.emberscene"};
    Time m_time;
    InputMap m_inputMap = InputMap::CreateDefaultMovementMap();
    ProjectConfiguration m_projectConfiguration;
    std::filesystem::path m_projectFilePath{"ember.emberproject"};
    bool m_projectLoadFailed = false;
    std::vector<std::string> m_customInputActions;
    std::string m_rebindAction;
    bool m_replaceActionOnRebind = true;
    char m_projectName[128] = "Untitled Project";
    char m_exportOutputDirectory[1024]{};
    char m_assetDirectory[260] = "assets";
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
    bool m_showPhysicsDebugOverlay = false;
    bool m_showScriptDiagnostics = true;
    bool m_showLogWindow = true;
    bool m_showScriptSourceEditor = false;
    bool m_scriptSourceDirty = false;
    bool m_allowScriptSourceClose = false;
    std::string m_scriptSourceAssetId;
    std::string m_scriptSourceOriginal;
    std::string m_scriptValidationMessage;
    std::size_t m_scriptValidationLine = 0;
    std::size_t m_scriptValidationColumn = 0;
    bool m_hasScriptValidationResult = false;
    bool m_scriptValidationSucceeded = false;
    std::filesystem::path m_scriptSourcePath;
    std::array<char, 65536> m_scriptSourceBuffer{};
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
    char m_scenePath[260] = "assets/scenes/untitled.emberscene";
    char m_assetSearch[128]{};
    char m_scriptImportSource[260]{};
    int m_assetTypeFilter = 0;
    char m_statusMessage[256] = "Ready";
};
} // namespace ember
