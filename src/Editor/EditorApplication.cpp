#include "Editor/EditorApplication.hpp"

#include "Engine/Scene/SceneSerializer.hpp"
#include "Engine/Core/FileSystem.hpp"
#include "Engine/Physics/PhysicsComponentRegistration.hpp"
#include "Engine/Physics/PhysicsComponents2D.hpp"
#include "Engine/Scene/AnimationComponents2D.hpp"
#include "Engine/Scene/AnimationComponentRegistration.hpp"
#include "Engine/Scene/AudioComponentRegistration.hpp"
#include "Engine/Scripting/ScriptComponentRegistration.hpp"
#include "Engine/Scripting/LuaScriptRuntime.hpp"
#include "Engine/UI/UIComponentRegistration.hpp"
#include "Engine/UI/UILayout2D.hpp"
#include "Engine/Platform/Input.hpp"
#include "Engine/Project/ProjectConfigurationStore.hpp"
#include "Engine/Project/ProjectExporter.hpp"
#include "Engine/Project/ProjectTemplateService.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <vector>
#include <string>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{
class EditorNoteComponent final : public ember::Component
{
public:
    std::string text = "New note";
    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<EditorNoteComponent>(*this);
    }
};

constexpr int kWindowWidth = 1440;
constexpr int kWindowHeight = 900;

const char* AssetTypeName(ember::AssetType type)
{
    switch (type)
    {
    case ember::AssetType::Texture: return "Texture";
    case ember::AssetType::Audio: return "Audio";
    case ember::AssetType::Scene: return "Scene";
    case ember::AssetType::Script: return "Script";
    }
    return "Unknown";
}

bool MatchesAssetType(ember::AssetType type, int filter)
{
    return filter == 0 ||
        (filter == 1 && type == ember::AssetType::Texture) ||
        (filter == 2 && type == ember::AssetType::Audio) ||
        (filter == 3 && type == ember::AssetType::Scene) ||
    (filter == 4 && type == ember::AssetType::Script);
}

float SnapValue(float value, float increment)
{
    return increment > 0.0F ? std::round(value / increment) * increment : value;
}

std::optional<std::filesystem::path> ResolveProjectScenePath(
    const std::filesystem::path& projectFilePath,
    const ember::ProjectConfiguration& projectConfiguration,
    const std::filesystem::path& scenePath)
{
    ember::ProjectConfiguration candidate = projectConfiguration;
    candidate.startupScene = scenePath;
    return ember::ProjectConfigurationStore::ResolveStartupScene(
        projectFilePath, candidate);
}

bool IsDefaultInputAction(std::string_view action)
{
    return action == "MoveLeft" || action == "MoveRight" ||
        action == "MoveUp" || action == "MoveDown";
}

std::string FindConflictingAction(
    const ember::InputMap& inputMap, SDL_Scancode key, std::string_view ignoredAction)
{
    for (const auto& [action, keys] : inputMap.GetAllBindings())
    {
        if (action == ignoredAction) continue;
        if (std::find(keys.begin(), keys.end(), key) != keys.end()) return action;
    }
    return {};
}

}

namespace ember
{
EditorApplication::~EditorApplication()
{
    Shutdown();
}

bool EditorApplication::Initialize(const std::filesystem::path& projectFile)
{
    if (!projectFile.empty()) m_projectFilePath = std::filesystem::absolute(projectFile).lexically_normal();
    m_logger.AddSink(std::make_shared<ConsoleLogSink>());

    if (!projectFile.empty())
    {
        std::string error;
        const auto configuration = ProjectConfigurationStore::Load(m_projectFilePath, &error);
        if (!configuration)
        {
            m_logger.Error("Project", "Could not open command-line project: " + error);
            std::cerr << "EMBER Editor: could not open project '"
                      << PathToUtf8(m_projectFilePath) << "': " << error << '\n';
            return false;
        }
    }

    char* preferencePath = SDL_GetPrefPath("EMBER", "EMBER");
    if (preferencePath != nullptr)
    {
        const std::filesystem::path logDirectory =
            std::filesystem::u8path(preferencePath) / "logs";
        SDL_free(preferencePath);
        std::error_code logDirectoryError;
        std::filesystem::create_directories(logDirectory, logDirectoryError);
        if (!logDirectoryError)
        {
            auto editorFileSink = std::make_shared<FileLogSink>(
                logDirectory / "ember-editor.log");
            if (editorFileSink->IsOpen())
                m_logger.AddSink(std::move(editorFileSink));
            else
                m_logger.Warning("Logging", "Could not open the Editor log file; console logging remains active");

            auto runtimeFileSink = std::make_shared<FileLogSink>(
                logDirectory / "ember-runtime.log");
            if (runtimeFileSink->IsOpen())
                m_runtimeLogger.AddSink(std::move(runtimeFileSink));
            else
                m_logger.Warning("Logging", "Could not open the game-runtime log file");
        }
        else
        {
            m_logger.Warning("Logging", "Could not create the per-user log directory; console logging remains active");
        }
    }
    else
    {
        m_logger.Warning("Logging", "Could not resolve the per-user log directory; console logging remains active");
    }

    m_logger.Info("Editor", "Initializing EMBER Editor");
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        m_logger.Error("Platform", std::string("SDL initialization failed: ") + SDL_GetError());
        std::cerr << "EMBER Editor: SDL initialization failed: " << SDL_GetError() << '\n';
        return false;
    }

    if (!m_audio.Initialize())
    {
        m_logger.Warning("Audio", "Audio device unavailable; continuing without audio");
        std::cerr << "EMBER Editor: Audio device unavailable; continuing without audio.\n";
    }

    m_window = SDL_CreateWindow("EMBER Editor", kWindowWidth, kWindowHeight, SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr)
    {
        m_logger.Error("Platform", std::string("Window creation failed: ") + SDL_GetError());
        std::cerr << "EMBER Editor: Window creation failed: " << SDL_GetError() << '\n';
        Shutdown();
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (m_renderer == nullptr)
    {
        m_logger.Error("Renderer", std::string("Renderer creation failed: ") + SDL_GetError());
        std::cerr << "EMBER Editor: Renderer creation failed: " << SDL_GetError() << '\n';
        Shutdown();
        return false;
    }

    (void)SDL_SetRenderVSync(m_renderer, 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer))
    {
        std::cerr << "EMBER Editor: ImGui SDL3 platform initialization failed.\n";
        Shutdown();
        return false;
    }
    if (!ImGui_ImplSDLRenderer3_Init(m_renderer))
    {
        std::cerr << "EMBER Editor: ImGui SDL renderer initialization failed.\n";
        Shutdown();
        return false;
    }

    m_componentRegistry.Register<EditorNoteComponent>("ember.note", "Note",
        [](const ember::Component& component) -> nlohmann::json
        {
            return nlohmann::json{{"text", static_cast<const EditorNoteComponent&>(component).text}};
        },
        [](ember::Component& component, const nlohmann::json& data)
        {
            if (!data.is_object() || !data.contains("text") || !data.at("text").is_string()) return false;
            const std::string text = data.at("text").get<std::string>();
            if (text.size() > 4096) return false;
            static_cast<EditorNoteComponent&>(component).text = text;
            return true;
        });
    (void)m_componentRegistry.RegisterProperty<EditorNoteComponent>(
        "ember.note", "text", "Text", &EditorNoteComponent::text);

    if (!RegisterPhysicsComponents(m_componentRegistry))
    {
        std::cerr << "EMBER Editor: Physics component registration failed.\n";
        return false;
    }

    if (!RegisterAnimationComponents(m_componentRegistry))
    {
        std::cerr << "EMBER Editor: Animation component registration failed.\n";
        return false;
    }

    if (!RegisterAudioComponents(m_componentRegistry))
    {
        std::cerr << "EMBER Editor: Audio component registration failed.\n";
        return false;
    }

    if (!RegisterScriptComponent(m_componentRegistry))
    {
        std::cerr << "EMBER Editor: Script component registration failed.\n";
        return false;
    }

    if (!RegisterUIComponents(m_componentRegistry))
    {
        std::cerr << "EMBER Editor: UI component registration failed.\n";
        return false;
    }

    LoadInputSettings();
    RefreshAssets();
    LoadStartupScene();
    ResetSceneHistory();
    m_isRunning = true;
    m_logger.Info("Editor", "EMBER Editor initialized successfully");
    return true;
}

void EditorApplication::Run()
{
    m_time.Reset();
    while (m_isRunning)
    {
        ProcessEvents();
        m_time.Tick();
        UpdatePlayMode();
        Render();
    }
}

void EditorApplication::UpdatePlayMode()
{
    if (!m_isPlaying)
    {
        return;
    }

    m_inputMap.Update();
    m_playScene.Update(std::clamp(m_time.DeltaSeconds(), 0.0F, 0.05F),
        m_inputMap.GetActionState());

    Entity* entity = m_playScene.FindEntity(m_selectedEntity);
    if (entity == nullptr || !entity->IsActive())
    {
        return;
    }

    const MovementComponent2D& movement = entity->GetMovementComponent();
    if (!movement.enabled || movement.speed <= 0.0F)
    {
        return;
    }

    Vector2 direction{};
    if (m_inputMap.IsActionDown("MoveLeft")) direction.x -= 1.0F;
    if (m_inputMap.IsActionDown("MoveRight")) direction.x += 1.0F;
    if (m_inputMap.IsActionDown("MoveUp")) direction.y -= 1.0F;
    if (m_inputMap.IsActionDown("MoveDown")) direction.y += 1.0F;

    const float lengthSquared = direction.x * direction.x + direction.y * direction.y;
    if (lengthSquared <= 0.0F)
    {
        return;
    }

    const float inverseLength = 1.0F / std::sqrt(lengthSquared);
    const float deltaSeconds = std::clamp(m_time.DeltaSeconds(), 0.0F, 0.05F);
    auto& position = entity->GetTransform().position;
    position.x += direction.x * inverseLength * movement.speed * deltaSeconds;
    position.y += direction.y * inverseLength * movement.speed * deltaSeconds;
}

void EditorApplication::Shutdown()
{
    if (m_isPlaying) StopPlayMode();
    if (m_isPlaying)
    {
        m_playScene.Stop();
        m_playScene.Clear();
        m_isPlaying = false;
    }
    StopAudioPreview();
    m_audio.Shutdown();
    ClearPreviewTextures();
    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    if (m_renderer != nullptr)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
    m_isRunning = false;
}

void SDLCALL EditorApplication::OpenProjectDialogCallback(
    void* userdata, const char* const* filelist, int filter)
{
    (void)filter;
    auto* application = static_cast<EditorApplication*>(userdata);
    if (application == nullptr) return;
    if (filelist == nullptr)
    {
        std::snprintf(application->m_statusMessage, sizeof(application->m_statusMessage),
            "Open Project cancelled");
        return;
    }
    if (filelist[0] != nullptr)
        application->OpenProjectFromPath(PathFromUtf8(filelist[0]));
}

void EditorApplication::OpenProjectFromPath(const std::filesystem::path& path)
{
    if (m_isPlaying || m_sceneDirty || m_scriptSourceDirty)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Save or discard current changes before opening another project");
        return;
    }

    const auto absolutePath = std::filesystem::absolute(path).lexically_normal();
    std::string error;
    const auto configuration = ProjectConfigurationStore::Load(absolutePath, &error);
    if (!configuration)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Open Project failed: %.190s", error.c_str());
        m_logger.Error("Project", error);
        return;
    }

    StopAudioPreview();
    ClearPreviewTextures();
    m_projectFilePath = absolutePath;
    m_projectConfiguration = *configuration;
    m_projectLoadFailed = false;
    m_scene = Scene("Untitled Scene");
    m_scene.SetLogger(&m_logger);
    m_currentScenePath.clear();
    m_selectedEntity = 0;
    m_entitySelection.Clear();
    m_sceneDirty = false;
    m_selectedAssetId.clear();
    m_importSettingsAssetId.clear();
    m_showScriptSourceEditor = false;
    m_scriptSourceDirty = false;
    m_scriptSourceAssetId.clear();
    m_scriptSourcePath.clear();
    m_scriptSourceOriginal.clear();
    m_scriptSourceBuffer.fill('\0');
    m_hasScriptValidationResult = false;
    m_scriptValidationMessage.clear();
    m_scriptValidationLine = 0;
    m_scriptValidationColumn = 0;
    LoadInputSettings();
    if (m_projectLoadFailed) return;
    RefreshAssets();
    LoadStartupScene();
    ResetSceneHistory();
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Opened project: %.190s", PathToUtf8(absolutePath).c_str());
    m_logger.Info("Project", "Opened project: " + PathToUtf8(absolutePath));
}

void EditorApplication::ProcessEvents()
{
    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT ||
            (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
             event.window.windowID == SDL_GetWindowID(m_window)))
        {
            RequestExit();
        }
        else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
        {
            if (!m_rebindAction.empty())
            {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    m_rebindAction.clear();
                    std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Key binding cancelled");
                }
                else if (event.key.scancode != SDL_SCANCODE_UNKNOWN)
                {
                    const std::string action = m_rebindAction;
                    InputMap candidate = m_inputMap;
                    if (m_replaceActionOnRebind)
                    {
                        candidate.ClearAction(action);
                    }
                    const std::string conflict =
                        FindConflictingAction(candidate, event.key.scancode, action);
                    const char* keyName = SDL_GetScancodeName(event.key.scancode);
                    bool added = false;
                    if (!conflict.empty())
                    {
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "Key already belongs to %.150s", conflict.c_str());
                    }
                    else
                    {
                        added = candidate.BindAction(action, event.key.scancode);
                        if (added)
                        {
                            m_inputMap = std::move(candidate);
                            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                "Bound %.100s to %.100s", action.c_str(),
                                keyName != nullptr && keyName[0] != '\0' ? keyName : "Unknown");
                        }
                        else
                        {
                            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                "That key is already bound to this action");
                        }
                    }
                    m_rebindAction.clear();
                    if (added && !SaveInputSettings())
                    {
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "Binding changed but project settings could not be saved");
                    }
                }
                continue;
            }

            const ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureKeyboard && !io.WantTextInput)
            {
                if (!m_isPlaying && (event.key.mod & SDL_KMOD_CTRL) != 0 &&
                    event.key.key == SDLK_A)
                {
                    SelectAllEntities();
                }
                else if (!m_isPlaying && (event.key.mod & SDL_KMOD_CTRL) != 0 &&
                    event.key.key == SDLK_Z && (event.key.mod & SDL_KMOD_SHIFT) != 0)
                {
                    RedoScene();
                }
                else if (!m_isPlaying && (event.key.mod & SDL_KMOD_CTRL) != 0 &&
                    event.key.key == SDLK_Z)
                {
                    UndoScene();
                }
                else if (!m_isPlaying && (event.key.mod & SDL_KMOD_CTRL) != 0 &&
                    event.key.key == SDLK_Y)
                {
                    RedoScene();
                }
                else if (!m_isPlaying && event.key.key == SDLK_S &&
                    (event.key.mod & SDL_KMOD_CTRL) != 0)
                {
                    (void)SaveScene();
                }
                else if (event.key.key == SDLK_F5 && (event.key.mod & SDL_KMOD_SHIFT) != 0)
                {
                    StopPlayMode();
                }
                else if (event.key.key == SDLK_F5)
                {
                    StartPlayMode();
                }
                else if (!m_isPlaying && event.key.key == SDLK_D &&
                    (event.key.mod & SDL_KMOD_CTRL) != 0)
                {
                    DuplicateSelectedEntity();
                }
                else if (!m_isPlaying && event.key.key == SDLK_DELETE)
                {
                    DeleteSelectedEntity();
                }
            }
        }
    }
}

void EditorApplication::Render()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    DrawMenuBar();
    DrawExportWindowsModal();
    DrawNewProjectModal();
    DrawSceneHierarchy();
    DrawInspector();
    DrawAssetBrowser();
    DrawInputSettings();
    DrawProjectSettings();
    DrawScriptDiagnostics();
    DrawLogWindow();
    DrawScriptSourceEditor();
    DrawViewport();
    DrawUnsavedChangesModal();
    DrawUnsavedScriptExitModal();

    ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(1440.0F, 24.0F), ImGuiCond_Always);
    if (ImGui::Begin("Status", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::Text("%s", m_statusMessage);
    }
    ImGui::End();

    ImGui::Render();
    SDL_SetRenderDrawColor(m_renderer, 24, 27, 36, 255);
    SDL_RenderClear(m_renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
    SDL_RenderPresent(m_renderer);
}

void EditorApplication::DrawExportWindowsModal()
{
    if (!ImGui::BeginPopupModal("Export Windows Game", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::TextWrapped("Create a standalone Windows game folder containing the EMBER runtime, project and assets.");
    ImGui::Spacing();
    ImGui::Text("Output directory");
    ImGui::SetNextItemWidth(520.0F);
    ImGui::InputText("##ExportOutputDirectory", m_exportOutputDirectory,
        sizeof(m_exportOutputDirectory));
    ImGui::TextDisabled("Choose a new or empty folder outside the source project.");

    if (m_sceneDirty)
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0F, 0.72F, 0.25F, 1.0F),
            "Save the current scene before exporting.");
    }
    if (m_isPlaying)
    {
        ImGui::TextColored(ImVec4(1.0F, 0.72F, 0.25F, 1.0F),
            "Stop Play Mode before exporting.");
    }

    ImGui::Separator();
    const bool canExport = !m_isPlaying && !m_sceneDirty &&
        !m_projectLoadFailed && m_exportOutputDirectory[0] != '\0';
    ImGui::BeginDisabled(!canExport);
    if (ImGui::Button("Export", ImVec2(120.0F, 0.0F)))
    {
        const char* executableDirectory = SDL_GetBasePath();
        std::filesystem::path runtimePath;
        if (executableDirectory != nullptr)
        {
            runtimePath = PathFromUtf8(executableDirectory);
            runtimePath /= "ember_runtime.exe";
        }

        ProjectExportResult result;
        std::string error;
        if (runtimePath.empty() ||
            !ProjectExporter::ExportWindows(m_projectFilePath, runtimePath,
                PathFromUtf8(m_exportOutputDirectory), &result, &error))
        {
            if (error.empty()) error = "Could not locate ember_runtime.exe beside the Editor.";
            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                "Windows export failed: %.190s", error.c_str());
            m_logger.Error("Export", error);
        }
        else
        {
            const std::string exportedPath = PathToUtf8(result.outputDirectory);
            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                "Windows game exported: %.190s", exportedPath.c_str());
            m_logger.Info("Export", "Windows game export completed: " + exportedPath);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120.0F, 0.0F)))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

void EditorApplication::DrawNewProjectModal()
{
    if (!ImGui::BeginPopupModal("Create New Project", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::TextWrapped("Create a new EMBER project from a built-in 2D template.");
    ImGui::Spacing();
    ImGui::Text("Project name");
    ImGui::SetNextItemWidth(420.0F);
    ImGui::InputText("##NewProjectName", m_newProjectName, sizeof(m_newProjectName));
    ImGui::Text("Project directory");
    ImGui::SetNextItemWidth(520.0F);
    ImGui::InputText("##NewProjectDirectory", m_newProjectDirectory,
        sizeof(m_newProjectDirectory));
    const char* templateNames[] = {"Blank 2D", "2D Starter"};
    ImGui::SetNextItemWidth(240.0F);
    ImGui::Combo("Template", &m_newProjectTemplate, templateNames,
        static_cast<int>(std::size(templateNames)));
    if (m_newProjectTemplate == 0)
        ImGui::TextDisabled("An empty scene and assets folder.");
    else
        ImGui::TextDisabled("A visible Player entity with WASD movement enabled.");

    ImGui::Separator();
    const bool canCreate = m_newProjectName[0] != '\0' &&
        m_newProjectDirectory[0] != '\0' && !m_isPlaying &&
        !m_sceneDirty && !m_scriptSourceDirty;
    ImGui::BeginDisabled(!canCreate);
    if (ImGui::Button("Create Project", ImVec2(150.0F, 0.0F)))
    {
        ProjectTemplateResult result;
        std::string error;
        const auto templateId = m_newProjectTemplate == 1
            ? ProjectTemplateId::Starter2D : ProjectTemplateId::Blank2D;
        if (!ProjectTemplateService::CreateProject(
                PathFromUtf8(m_newProjectDirectory), m_newProjectName,
                templateId, &result, &error))
        {
            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                "Project creation failed: %.180s", error.c_str());
            m_logger.Error("Project", error);
        }
        else
        {
            m_projectFilePath = result.projectFile;
            m_projectLoadFailed = false;
            m_sceneDirty = false;
            m_selectedEntity = 0;
            m_entitySelection.Clear();
            m_scriptSourceDirty = false;
            m_showScriptSourceEditor = false;
            m_scriptSourceAssetId.clear();
            m_scriptSourcePath.clear();
            m_scriptSourceOriginal.clear();
            m_scriptValidationMessage.clear();
            LoadInputSettings();
            LoadStartupScene();
            RefreshAssets();
            const std::string createdPath = PathToUtf8(result.projectDirectory);
            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                "Created project: %.190s", createdPath.c_str());
            m_logger.Info("Project", "Created project: " + createdPath);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120.0F, 0.0F)))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

void EditorApplication::DrawMenuBar()
{
    bool openNewProjectPopup = false;
    bool openExportPopup = false;
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        const bool canOpenProject = !m_isPlaying && !m_sceneDirty && !m_scriptSourceDirty;
        if (ImGui::MenuItem("Open Project...", nullptr, false, canOpenProject))
        {
            const SDL_DialogFileFilter filters[]{{"EMBER Project", "emberproject"}};
            const std::string directory = PathToUtf8(m_projectFilePath.parent_path());
            SDL_ShowOpenFileDialog(&EditorApplication::OpenProjectDialogCallback,
                this, m_window, filters, 1, directory.c_str(), false);
        }
        ImGui::Separator();
        const bool canCreateProject = !m_isPlaying && !m_sceneDirty && !m_scriptSourceDirty;
        if (ImGui::MenuItem("New Project...", nullptr, false, canCreateProject))
        {
            std::snprintf(m_newProjectName, sizeof(m_newProjectName), "%s", "New Project");
            const auto defaultDirectory = std::filesystem::current_path() / "New Project";
            std::snprintf(m_newProjectDirectory, sizeof(m_newProjectDirectory), "%s",
                PathToUtf8(defaultDirectory).c_str());
            m_newProjectTemplate = 0;
            openNewProjectPopup = true;
        }
        ImGui::Separator();
        ImGui::BeginDisabled(m_isPlaying);
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) (void)SaveScene();
        if (ImGui::MenuItem("Load Scene")) LoadScene();
        ImGui::EndDisabled();
        ImGui::Separator();
        const bool canExport = !m_isPlaying && !m_projectLoadFailed &&
            !m_projectConfiguration.startupScene.empty();
        if (ImGui::MenuItem("Export Windows Game...", nullptr, false, canExport))
        {
            const auto projectPath = std::filesystem::absolute(m_projectFilePath).lexically_normal();
            const auto projectRoot = projectPath.parent_path();
            auto outputName = projectRoot.filename().string();
            if (outputName.empty()) outputName = "EMBER-Game";
            const auto defaultOutput = projectRoot.parent_path() / (outputName + "-Windows");
            std::snprintf(m_exportOutputDirectory, sizeof(m_exportOutputDirectory), "%s",
                PathToUtf8(defaultOutput).c_str());
            openExportPopup = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) RequestExit();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::BeginDisabled(m_isPlaying || !m_sceneUndoHistory.CanUndo());
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) UndoScene();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(m_isPlaying || !m_sceneUndoHistory.CanRedo());
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) RedoScene();
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Entity"))
    {
        if (ImGui::MenuItem("Create Empty Entity")) CreateEntity();
        ImGui::Separator();
        ImGui::BeginDisabled(m_isPlaying || m_scene.IsEmpty());
        if (ImGui::MenuItem("Select All", "Ctrl+A")) SelectAllEntities();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(m_isPlaying || m_entitySelection.Empty());
        if (ImGui::MenuItem("Duplicate Selected", "Ctrl+D")) DuplicateSelectedEntity();
        if (ImGui::MenuItem("Delete Selected", "Delete")) DeleteSelectedEntity();
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene"))
    {
        if (!m_isPlaying && ImGui::MenuItem("Play", "F5")) StartPlayMode();
        if (m_isPlaying && ImGui::MenuItem("Stop", "Shift+F5")) StopPlayMode();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        ImGui::MenuItem("Log", nullptr, &m_showLogWindow);
        ImGui::MenuItem("Script Diagnostics", nullptr, &m_showScriptDiagnostics);
        if (ImGui::MenuItem("Script Source Editor", nullptr, m_showScriptSourceEditor))
        {
            if (m_showScriptSourceEditor && m_scriptSourceDirty)
            {
                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                    "Save or discard Script changes before closing the editor");
            }
            else
            {
                m_showScriptSourceEditor = !m_showScriptSourceEditor;
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Assets"))
    {
        if (ImGui::MenuItem("Refresh Asset Browser")) RefreshAssets();
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    // Open modal popups at the root ID scope used by BeginPopupModal.
    if (openNewProjectPopup) ImGui::OpenPopup("Create New Project");
    if (openExportPopup) ImGui::OpenPopup("Export Windows Game");
}

void EditorApplication::DrawSceneHierarchy()
{
    ImGui::SetNextWindowPos(ImVec2(0.0F, 24.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280.0F, 590.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Hierarchy"))
    {
        Scene& activeScene = GetActiveScene();
        ImGui::Text("%s%s", activeScene.GetName().c_str(),
            (!m_isPlaying && m_sceneDirty) ? " *" : "");
        ImGui::SameLine();
        ImGui::BeginDisabled(m_isPlaying);
        if (ImGui::SmallButton("+")) CreateEntity();
        ImGui::EndDisabled();
        ImGui::Separator();

        const std::vector<EntityId> orderedIds = activeScene.GetEntityIds();
        const ImGuiIO& selectionIO = ImGui::GetIO();
        const auto drawHierarchyEntity = [this, &activeScene, &orderedIds, &selectionIO]
            (auto&& drawSelf, EntityId id) -> void
        {
            Entity* entity = activeScene.FindEntity(id);
            if (entity == nullptr) return;

            char label[160]{};
            std::snprintf(label, sizeof(label), "%s##%llu",
                entity->GetName().c_str(), static_cast<unsigned long long>(id));
            if (ImGui::Selectable(label, m_entitySelection.Contains(id)))
            {
                if (selectionIO.KeyShift)
                    m_entitySelection.SelectRange(orderedIds, id, selectionIO.KeyCtrl);
                else if (selectionIO.KeyCtrl)
                    m_entitySelection.Toggle(id);
                else
                    m_entitySelection.SelectOnly(id);
                m_selectedEntity = m_entitySelection.Primary();
            }

            if (!m_isPlaying && ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("EMBER_ENTITY_ID", &id, sizeof(id));
                ImGui::TextUnformatted(entity->GetName().c_str());
                ImGui::EndDragDropSource();
            }
            if (!m_isPlaying && ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EMBER_ENTITY_ID"))
                {
                    if (payload->DataSize == static_cast<int>(sizeof(EntityId)))
                    {
                        const EntityId draggedId = *static_cast<const EntityId*>(payload->Data);
                        if (activeScene.SetParent(draggedId, id)) MarkSceneDirty();
                    }
                }
                ImGui::EndDragDropTarget();
            }
            if (ImGui::BeginPopupContextItem("EntityHierarchyContext"))
            {
                const bool hasParent = entity->GetParentId() != 0;
                ImGui::BeginDisabled(!hasParent || m_isPlaying);
                if (ImGui::MenuItem("Unparent") &&
                    activeScene.SetParent(id, 0)) MarkSceneDirty();
                ImGui::EndDisabled();
                ImGui::EndPopup();
            }

            ImGui::Indent(14.0F);
            for (const EntityId childId : activeScene.GetChildren(id))
                drawSelf(drawSelf, childId);
            ImGui::Unindent(14.0F);
        };
        for (const EntityId rootId : activeScene.GetRootEntityIds())
            drawHierarchyEntity(drawHierarchyEntity, rootId);

        if (activeScene.IsEmpty()) ImGui::TextDisabled("No entities. Press + to create one.");
        else if (!m_isPlaying)
        {
            if (m_entitySelection.Size() > 1)
                ImGui::TextDisabled("%zu entities selected", m_entitySelection.Size());
            ImGui::BeginDisabled(m_entitySelection.Empty());
            if (ImGui::Button("Duplicate Selected")) DuplicateSelectedEntity();
            ImGui::SameLine();
            if (ImGui::Button("Delete Selected")) DeleteSelectedEntity();
            ImGui::EndDisabled();
            ImGui::TextDisabled("Shortcuts: Ctrl/Shift+Click select | Ctrl+A all | Ctrl+D duplicate | Delete remove");
        }
        else
        {
            Entity* selectedEntity = activeScene.FindEntity(m_selectedEntity);
            const auto* audioSource = selectedEntity != nullptr
                ? selectedEntity->GetComponent<AudioSource2D>() : nullptr;
            if (audioSource != nullptr)
            {
                ImGui::SeparatorText("Audio Runtime");
                const bool hasRuntimeAudio = activeScene.HasAudioSource(m_selectedEntity);
                const bool isPlaying = activeScene.IsAudioSourcePlaying(m_selectedEntity);
                ImGui::TextDisabled("Asset: %s", audioSource->audioAssetId.empty()
                    ? "None" : audioSource->audioAssetId.c_str());
                ImGui::BeginDisabled(!hasRuntimeAudio);
                if (isPlaying)
                {
                    if (ImGui::Button("Stop Audio"))
                    {
                        if (activeScene.StopAudioSource(m_selectedEntity))
                            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                "Stopped AudioSource2D on Entity %llu",
                                static_cast<unsigned long long>(m_selectedEntity));
                    }
                }
                else if (ImGui::Button("Play Audio"))
                {
                    if (activeScene.PlayAudioSource(m_selectedEntity))
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "Played AudioSource2D on Entity %llu",
                            static_cast<unsigned long long>(m_selectedEntity));
                    else
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "Could not play AudioSource2D on Entity %llu",
                            static_cast<unsigned long long>(m_selectedEntity));
                }
                ImGui::EndDisabled();
                if (!hasRuntimeAudio)
                    ImGui::TextDisabled("No runtime sound loaded; check AudioSource2D settings and diagnostics.");
            }

            const auto& audioDiagnostics = m_playScene.GetAudioDiagnostics();
            if (!audioDiagnostics.empty() && ImGui::TreeNode("Audio Diagnostics"))
            {
                for (const std::string& diagnostic : audioDiagnostics)
                    ImGui::BulletText("%s", diagnostic.c_str());
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}

void EditorApplication::DrawInspector()
{
    ImGui::SetNextWindowPos(ImVec2(1080.0F, 24.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0F, 590.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Inspector"))
    {
        Scene& activeScene = GetActiveScene();
        ImGui::BeginDisabled(m_isPlaying);
        Entity* entity = activeScene.FindEntity(m_selectedEntity);
        if (m_entitySelection.Size() > 1)
        {
            std::vector<Entity*> selectedEntities;
            selectedEntities.reserve(m_entitySelection.Size());
            for (const EntityId id : m_entitySelection.GetIds())
                if (Entity* selected = activeScene.FindEntity(id))
                    selectedEntities.push_back(selected);

            ImGui::Text("%zu entities selected", selectedEntities.size());
            if (selectedEntities.size() != m_entitySelection.Size())
                ImGui::TextDisabled("Selection changed; missing Entities will be pruned.");
            if (!selectedEntities.empty())
            {
                const auto applyAll = [&selectedEntities](auto&& edit)
                {
                    for (Entity* selected : selectedEntities) edit(*selected);
                };
                const auto showMixed = [](bool mixed)
                {
                    if (mixed) ImGui::TextDisabled("Mixed");
                };

                const std::string& firstName = selectedEntities.front()->GetName();
                bool nameMixed = false;
                for (const Entity* selected : selectedEntities)
                    nameMixed = nameMixed || selected->GetName() != firstName;
                char sharedName[128]{};
                if (!nameMixed)
                    std::snprintf(sharedName, sizeof(sharedName), "%s", firstName.c_str());
                showMixed(nameMixed);
                if (nameMixed) ImGui::SameLine();
                if (ImGui::InputText("Name", sharedName, sizeof(sharedName)))
                {
                    const std::string name = sharedName;
                    applyAll([&name](Entity& selected) { selected.SetName(name); });
                    MarkSceneDirty();
                }

                bool active = selectedEntities.front()->IsActive();
                bool activeMixed = false;
                for (const Entity* selected : selectedEntities)
                    activeMixed = activeMixed || selected->IsActive() != active;
                showMixed(activeMixed);
                if (activeMixed) ImGui::SameLine();
                if (ImGui::Checkbox("Active", &active))
                {
                    applyAll([active](Entity& selected) { selected.SetActive(active); });
                    MarkSceneDirty();
                }

                ImGui::SeparatorText("Transform (Local)");
                Vector2 position = selectedEntities.front()->GetTransform().position;
                Vector2 scale = selectedEntities.front()->GetTransform().scale;
                float rotation = selectedEntities.front()->GetTransform().rotation;
                bool positionMixed = false, scaleMixed = false, rotationMixed = false;
                for (const Entity* selected : selectedEntities)
                {
                    const auto& value = selected->GetTransform();
                    positionMixed = positionMixed || value.position.x != position.x || value.position.y != position.y;
                    scaleMixed = scaleMixed || value.scale.x != scale.x || value.scale.y != scale.y;
                    rotationMixed = rotationMixed || value.rotation != rotation;
                }
                showMixed(positionMixed);
                if (positionMixed) ImGui::SameLine();
                if (ImGui::DragFloat2("Position", &position.x, 0.1F))
                {
                    applyAll([position](Entity& selected) { selected.GetTransform().position = position; });
                    MarkSceneDirty();
                }
                showMixed(scaleMixed);
                if (scaleMixed) ImGui::SameLine();
                if (ImGui::DragFloat2("Scale", &scale.x, 0.01F))
                {
                    applyAll([scale](Entity& selected) { selected.GetTransform().scale = scale; });
                    MarkSceneDirty();
                }
                showMixed(rotationMixed);
                if (rotationMixed) ImGui::SameLine();
                if (ImGui::DragFloat("Rotation (rad)", &rotation, 0.01F))
                {
                    applyAll([rotation](Entity& selected) { selected.GetTransform().rotation = rotation; });
                    MarkSceneDirty();
                }

                ImGui::SeparatorText("Movement");
                bool movementEnabled = selectedEntities.front()->GetMovementComponent().enabled;
                float movementSpeed = selectedEntities.front()->GetMovementComponent().speed;
                bool movementEnabledMixed = false, movementSpeedMixed = false;
                for (const Entity* selected : selectedEntities)
                {
                    const auto& value = selected->GetMovementComponent();
                    movementEnabledMixed = movementEnabledMixed || value.enabled != movementEnabled;
                    movementSpeedMixed = movementSpeedMixed || value.speed != movementSpeed;
                }
                showMixed(movementEnabledMixed);
                if (movementEnabledMixed) ImGui::SameLine();
                if (ImGui::Checkbox("Enable keyboard movement", &movementEnabled))
                {
                    applyAll([movementEnabled](Entity& selected) {
                        selected.GetMovementComponent().enabled = movementEnabled;
                    });
                    MarkSceneDirty();
                }
                showMixed(movementSpeedMixed);
                if (movementSpeedMixed) ImGui::SameLine();
                if (ImGui::DragFloat("Movement speed", &movementSpeed, 1.0F, 0.0F, 2000.0F))
                {
                    applyAll([movementSpeed](Entity& selected) {
                        selected.GetMovementComponent().speed = movementSpeed;
                    });
                    MarkSceneDirty();
                }

                ImGui::SeparatorText("Renderer");
                bool visible = selectedEntities.front()->GetRenderComponent().visible;
                Vector2 renderSize = selectedEntities.front()->GetRenderComponent().size;
                int tint[4]{
                    selectedEntities.front()->GetRenderComponent().red,
                    selectedEntities.front()->GetRenderComponent().green,
                    selectedEntities.front()->GetRenderComponent().blue,
                    selectedEntities.front()->GetRenderComponent().alpha
                };
                bool visibleMixed = false, sizeMixed = false, tintMixed = false;
                for (const Entity* selected : selectedEntities)
                {
                    const auto& value = selected->GetRenderComponent();
                    visibleMixed = visibleMixed || value.visible != visible;
                    sizeMixed = sizeMixed || value.size.x != renderSize.x || value.size.y != renderSize.y;
                    tintMixed = tintMixed || value.red != tint[0] || value.green != tint[1] ||
                        value.blue != tint[2] || value.alpha != tint[3];
                }
                showMixed(visibleMixed);
                if (visibleMixed) ImGui::SameLine();
                if (ImGui::Checkbox("Visible", &visible))
                {
                    applyAll([visible](Entity& selected) { selected.GetRenderComponent().visible = visible; });
                    MarkSceneDirty();
                }
                showMixed(sizeMixed);
                if (sizeMixed) ImGui::SameLine();
                if (ImGui::DragFloat2("Size", &renderSize.x, 0.5F, 0.0F, 4096.0F))
                {
                    applyAll([renderSize](Entity& selected) { selected.GetRenderComponent().size = renderSize; });
                    MarkSceneDirty();
                }
                showMixed(tintMixed);
                if (tintMixed) ImGui::SameLine();
                if (ImGui::InputInt4("Tint RGBA", tint))
                {
                    const int red = std::clamp(tint[0], 0, 255);
                    const int green = std::clamp(tint[1], 0, 255);
                    const int blue = std::clamp(tint[2], 0, 255);
                    const int alpha = std::clamp(tint[3], 0, 255);
                    applyAll([red, green, blue, alpha](Entity& selected) {
                        auto& render = selected.GetRenderComponent();
                        render.red = static_cast<std::uint8_t>(red);
                        render.green = static_cast<std::uint8_t>(green);
                        render.blue = static_cast<std::uint8_t>(blue);
                        render.alpha = static_cast<std::uint8_t>(alpha);
                    });
                    MarkSceneDirty();
                }

                ImGui::SeparatorText("Components on Selection");
                const auto registeredTypes = m_componentRegistry.GetRegisteredTypes();
                for (const auto& type : registeredTypes)
                {
                    std::vector<Component*> matchingComponents;
                    matchingComponents.reserve(selectedEntities.size());
                    for (Entity* selected : selectedEntities)
                    {
                        Component* match = nullptr;
                        for (const auto& candidate : selected->GetComponents())
                        {
                            const auto* candidateType = m_componentRegistry.Find(*candidate);
                            if (candidateType != nullptr && candidateType->id == type.id)
                            {
                                match = candidate.get();
                                break;
                            }
                        }
                        if (match != nullptr) matchingComponents.push_back(match);
                    }
                    const std::size_t presentCount = matchingComponents.size();
                    if (presentCount == 0) continue;

                    ImGui::PushID(type.id.c_str());
                    ImGui::Text("%s (%zu/%zu)", type.displayName.c_str(),
                        presentCount, selectedEntities.size());
                    if (presentCount < selectedEntities.size())
                    {
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Add to Missing"))
                        {
                            if (editor::AddComponentToEntities(selectedEntities, m_componentRegistry, type.id))
                                MarkSceneDirty();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Remove from Selection"))
                    {
                        if (editor::RemoveComponentFromEntities(selectedEntities, m_componentRegistry, type.id))
                            MarkSceneDirty();
                        ImGui::PopID();
                        continue;
                    }

                    if (presentCount == selectedEntities.size() && ImGui::TreeNode("Properties"))
                    {
                        const auto* properties = m_componentRegistry.FindProperties(*matchingComponents.front());
                        if (properties != nullptr)
                        {
                            for (const auto& property : *properties)
                            {
                                if ((property.id == "audio_asset_id" || property.id == "script_asset_id") &&
                                    property.type == ComponentPropertyType::String)
                                {
                                    ImGui::TextDisabled("%s: multi-edit asset selection is not supported yet",
                                        property.displayName.c_str());
                                    continue;
                                }

                                std::vector<nlohmann::json> values;
                                values.reserve(matchingComponents.size());
                                for (const Component* match : matchingComponents)
                                    values.push_back(property.get(*match));
                                const bool mixed = editor::HasMixedValues(values);
                                ImGui::PushID(property.id.c_str());
                                switch (property.type)
                                {
                                case ComponentPropertyType::Boolean:
                                {
                                    bool editedValue = values.front().get<bool>();
                                    showMixed(mixed);
                                    if (mixed) ImGui::SameLine();
                                    if (ImGui::Checkbox(property.displayName.c_str(), &editedValue))
                                    {
                                        if (editor::ApplyComponentPropertyAtomically(
                                            matchingComponents, property, nlohmann::json(editedValue)))
                                            MarkSceneDirty();
                                    }
                                    break;
                                }
                                case ComponentPropertyType::Integer:
                                {
                                    int editedValue = values.front().get<int>();
                                    showMixed(mixed);
                                    if (mixed) ImGui::SameLine();
                                    if (ImGui::DragInt(property.displayName.c_str(), &editedValue))
                                    {
                                        if (editor::ApplyComponentPropertyAtomically(
                                            matchingComponents, property, nlohmann::json(editedValue)))
                                            MarkSceneDirty();
                                    }
                                    break;
                                }
                                case ComponentPropertyType::Float:
                                {
                                    float editedValue = values.front().get<float>();
                                    showMixed(mixed);
                                    if (mixed) ImGui::SameLine();
                                    if (ImGui::DragFloat(property.displayName.c_str(), &editedValue, 0.05F))
                                    {
                                        if (editor::ApplyComponentPropertyAtomically(
                                            matchingComponents, property, nlohmann::json(editedValue)))
                                            MarkSceneDirty();
                                    }
                                    break;
                                }
                                case ComponentPropertyType::String:
                                {
                                    const std::string current = values.front().get<std::string>();
                                    char editedValue[4097]{};
                                    if (!mixed) std::snprintf(editedValue, sizeof(editedValue), "%s", current.c_str());
                                    showMixed(mixed);
                                    if (mixed) ImGui::SameLine();
                                    if (ImGui::InputText(property.displayName.c_str(), editedValue, sizeof(editedValue)))
                                    {
                                        if (editor::ApplyComponentPropertyAtomically(
                                            matchingComponents, property, nlohmann::json(std::string(editedValue))))
                                            MarkSceneDirty();
                                    }
                                    break;
                                }
                                case ComponentPropertyType::Vector2:
                                {
                                    float editedValue[2]{
                                        values.front().at(0).get<float>(), values.front().at(1).get<float>()};
                                    showMixed(mixed);
                                    if (mixed) ImGui::SameLine();
                                    if (ImGui::DragFloat2(property.displayName.c_str(), editedValue, 0.05F))
                                    {
                                        if (editor::ApplyComponentPropertyAtomically(
                                            matchingComponents, property,
                                            nlohmann::json::array({editedValue[0], editedValue[1]})))
                                            MarkSceneDirty();
                                    }
                                    break;
                                }
                                case ComponentPropertyType::Enumeration:
                                {
                                    const int current = values.front().get<int>();
                                    const char* preview = mixed ? "Mixed" : "Unknown";
                                    if (!mixed)
                                        for (const auto& option : property.enumValues)
                                            if (option.first == current) preview = option.second.c_str();
                                    if (ImGui::BeginCombo(property.displayName.c_str(), preview))
                                    {
                                        for (const auto& option : property.enumValues)
                                            if (ImGui::Selectable(option.second.c_str(), !mixed && option.first == current) &&
                                                editor::ApplyComponentPropertyAtomically(
                                                    matchingComponents, property, nlohmann::json(option.first)))
                                                MarkSceneDirty();
                                        ImGui::EndCombo();
                                    }
                                    break;
                                }
                                case ComponentPropertyType::EntityReference:
                                {
                                    const EntityId current = values.front().get<EntityId>();
                                    const char* preview = mixed ? "Mixed" : "None";
                                    if (!mixed && current != 0)
                                    {
                                        const Entity* target = activeScene.FindEntity(current);
                                        if (target != nullptr) preview = target->GetName().c_str();
                                    }
                                    if (ImGui::BeginCombo(property.displayName.c_str(), preview))
                                    {
                                        if (ImGui::Selectable("None", !mixed && current == 0) &&
                                            editor::ApplyComponentPropertyAtomically(
                                                matchingComponents, property, nlohmann::json(std::uint64_t{0})))
                                            MarkSceneDirty();
                                        for (const EntityId candidateId : activeScene.GetEntityIds())
                                        {
                                            const Entity* candidate = activeScene.FindEntity(candidateId);
                                            if (candidate == nullptr) continue;
                                            const std::string label = candidate->GetName() +
                                                " (#" + std::to_string(candidateId) + ")";
                                            if (ImGui::Selectable(label.c_str(), !mixed && candidateId == current) &&
                                                editor::ApplyComponentPropertyAtomically(
                                                    matchingComponents, property, nlohmann::json(candidateId)))
                                                MarkSceneDirty();
                                        }
                                        ImGui::EndCombo();
                                    }
                                    break;
                                }
                                }
                                ImGui::PopID();
                            }
                        }
                        ImGui::TreePop();
                    }
                    else if (presentCount < selectedEntities.size())
                    {
                        ImGui::TextDisabled("Add this Component to the missing Entities to edit shared properties.");
                    }
                    ImGui::PopID();
                }
                ImGui::TextDisabled("Registered Components can be added to missing Entities or removed from the selection.");
            }
        }
        else if (entity == nullptr)
        {
            ImGui::TextDisabled("Select an entity to inspect its properties.");
        }
        else
        {
            char name[128]{};
            std::snprintf(name, sizeof(name), "%s", entity->GetName().c_str());
            if (ImGui::InputText("Name", name, sizeof(name)))
            {
                entity->SetName(name);
                MarkSceneDirty();
            }

            bool active = entity->IsActive();
            if (ImGui::Checkbox("Active", &active))
            {
                entity->SetActive(active);
                MarkSceneDirty();
            }

            ImGui::SeparatorText("Transform");
            auto& transform = entity->GetTransform();
            if (ImGui::DragFloat2("Position", &transform.position.x, 0.1F)) MarkSceneDirty();
            if (ImGui::DragFloat2("Scale", &transform.scale.x, 0.01F)) MarkSceneDirty();
            if (ImGui::DragFloat("Rotation (rad)", &transform.rotation, 0.01F)) MarkSceneDirty();

            ImGui::SeparatorText("Movement");
            auto& movement = entity->GetMovementComponent();
            if (ImGui::Checkbox("Enable keyboard movement", &movement.enabled)) MarkSceneDirty();
            if (ImGui::DragFloat("Movement speed", &movement.speed, 1.0F, 0.0F, 2000.0F)) MarkSceneDirty();

            ImGui::SeparatorText("Renderer");
            auto& render = entity->GetRenderComponent();
            if (ImGui::Checkbox("Visible", &render.visible)) MarkSceneDirty();
            if (ImGui::DragFloat2("Size", &render.size.x, 0.5F, 0.0F, 4096.0F)) MarkSceneDirty();
            int color[4]{render.red, render.green, render.blue, render.alpha};
            if (ImGui::InputInt4("Tint RGBA", color))
            {
                render.red = static_cast<std::uint8_t>(std::clamp(color[0], 0, 255));
                render.green = static_cast<std::uint8_t>(std::clamp(color[1], 0, 255));
                render.blue = static_cast<std::uint8_t>(std::clamp(color[2], 0, 255));
                render.alpha = static_cast<std::uint8_t>(std::clamp(color[3], 0, 255));
                MarkSceneDirty();
            }

            ImGui::SeparatorText("Components");
            for (const auto& component : entity->GetComponents())
            {
                const auto* info = m_componentRegistry.Find(*component);
                const char* displayName = info ? info->displayName.c_str() : "Unknown Component";
                ImGui::PushID(component.get());
                if (ImGui::TreeNode(displayName))
                {
                    const auto* properties = m_componentRegistry.FindProperties(*component);
                    if (properties != nullptr)
                    {
                        for (const auto& property : *properties)
                        {
                            ImGui::PushID(property.id.c_str());
                            switch (property.type)
                            {
                            case ComponentPropertyType::Boolean:
                            {
                                bool value = property.get(*component).get<bool>();
                                if (ImGui::Checkbox(property.displayName.c_str(), &value) &&
                                    property.set(*component, nlohmann::json(value)))
                                    MarkSceneDirty();
                                break;
                            }
                            case ComponentPropertyType::Integer:
                            {
                                int value = property.get(*component).get<int>();
                                if (ImGui::DragInt(property.displayName.c_str(), &value) &&
                                    property.set(*component, nlohmann::json(value)))
                                    MarkSceneDirty();
                                break;
                            }
                            case ComponentPropertyType::Float:
                            {
                                float value = property.get(*component).get<float>();
                                if (ImGui::DragFloat(property.displayName.c_str(), &value, 0.05F) &&
                                    property.set(*component, nlohmann::json(value)))
                                    MarkSceneDirty();
                                break;
                            }
                            case ComponentPropertyType::String:
                            {
                                const std::string current = property.get(*component).get<std::string>();
                                if (property.id == "audio_asset_id")
                                {
                                    const AssetMetadata* currentAsset = m_assetRegistry.Find(current);
                                    const std::string preview = current.empty() ? "None"
                                        : (currentAsset != nullptr && currentAsset->type == AssetType::Audio
                                            ? currentAsset->source.generic_string()
                                            : "Missing Audio asset: " + current);
                                    if (ImGui::BeginCombo(property.displayName.c_str(), preview.c_str()))
                                    {
                                        if (ImGui::Selectable("None", current.empty()) &&
                                            property.set(*component, nlohmann::json(std::string{})))
                                            MarkSceneDirty();
                                        std::vector<const AssetMetadata*> audioAssets;
                                        for (const auto& [assetId, metadata] : m_assetRegistry.GetAssets())
                                        {
                                            (void)assetId;
                                            if (metadata.type != AssetType::Audio) continue;
                                            const auto path = m_assetRegistry.GetDirectory().Resolve(metadata.source);
                                            std::error_code pathError;
                                            if (!path || !std::filesystem::exists(*path, pathError) || pathError)
                                                continue;
                                            audioAssets.push_back(&metadata);
                                        }
                                        std::sort(audioAssets.begin(), audioAssets.end(),
                                            [](const AssetMetadata* left, const AssetMetadata* right)
                                            {
                                                return left->source.generic_string() < right->source.generic_string();
                                            });
                                        for (const AssetMetadata* metadata : audioAssets)
                                        {
                                            const std::string label = metadata->source.generic_string() + " (" +
                                                metadata->assetId + ")##" + metadata->assetId;
                                            const bool selected = metadata->assetId == current;
                                            if (ImGui::Selectable(label.c_str(), selected) &&
                                                property.set(*component, nlohmann::json(metadata->assetId)))
                                                MarkSceneDirty();
                                            if (selected) ImGui::SetItemDefaultFocus();
                                        }
                                        ImGui::EndCombo();
                                    }
                                    ImGui::TextDisabled("Choose an imported Audio asset from the project Asset Browser.");
                                }

                                else if (property.id == "script_asset_id")
                                {
                                    const AssetMetadata* currentAsset = m_assetRegistry.Find(current);
                                    const std::string preview = current.empty() ? "None"
                                        : (currentAsset != nullptr && currentAsset->type == AssetType::Script
                                            ? currentAsset->source.generic_string() + " [" +
                                                (currentAsset->scriptLanguage.empty() ? "Unassigned" : currentAsset->scriptLanguage) + "]"
                                            : "Missing Script asset: " + current);
                                    if (ImGui::BeginCombo(property.displayName.c_str(), preview.c_str()))
                                    {
                                        if (ImGui::Selectable("None", current.empty()) &&
                                            property.set(*component, nlohmann::json(std::string{})))
                                            MarkSceneDirty();
                                        std::vector<const AssetMetadata*> scriptAssets;
                                        for (const auto& [assetId, metadata] : m_assetRegistry.GetAssets())
                                        {
                                            (void)assetId;
                                            if (metadata.type != AssetType::Script) continue;
                                            const auto path = m_assetRegistry.GetDirectory().Resolve(metadata.source);
                                            std::error_code pathError;
                                            if (!path || !std::filesystem::is_regular_file(*path, pathError) || pathError)
                                                continue;
                                            scriptAssets.push_back(&metadata);
                                        }
                                        std::sort(scriptAssets.begin(), scriptAssets.end(),
                                            [](const AssetMetadata* left, const AssetMetadata* right)
                                            {
                                                return left->source.generic_string() < right->source.generic_string();
                                            });
                                        for (const AssetMetadata* metadata : scriptAssets)
                                        {
                                            const std::string label = metadata->source.generic_string() +
                                                " (" + metadata->assetId + ")##" + metadata->assetId;
                                            const bool selected = metadata->assetId == current;
                                            if (ImGui::Selectable(label.c_str(), selected) &&
                                                property.set(*component, nlohmann::json(metadata->assetId)))
                                                MarkSceneDirty();
                                            if (selected) ImGui::SetItemDefaultFocus();
                                        }
                                        ImGui::EndCombo();
                                    }
                                    if (currentAsset != nullptr && currentAsset->type == AssetType::Script &&
                                        currentAsset->scriptLanguage == "lua")
                                        ImGui::TextDisabled("Lua 5.5.1 backend is available for this Script in Play Mode.");
                                    else if (currentAsset != nullptr && currentAsset->type == AssetType::Script)
                                        ImGui::TextDisabled("No matching runtime backend is available yet. This Script will not execute in Play Mode.");
                                    else
                                        ImGui::TextDisabled("Choose an imported Script asset. The language tag selects its runtime backend.");
                                }
                                else
                                {
                                    char value[4097]{};
                                    std::snprintf(value, sizeof(value), "%s", current.c_str());
                                    if (ImGui::InputTextMultiline(property.displayName.c_str(), value,
                                        sizeof(value), ImVec2(-1.0F, 72.0F)) &&
                                        property.set(*component, nlohmann::json(value)))
                                        MarkSceneDirty();
                                }
                                break;
                            }
                            case ComponentPropertyType::Vector2:
                            {
                                const auto current = property.get(*component);
                                float value[2]{current.at(0).get<float>(), current.at(1).get<float>()};
                                if (ImGui::DragFloat2(property.displayName.c_str(), value, 0.05F) &&
                                    property.set(*component, nlohmann::json::array({value[0], value[1]})))
                                    MarkSceneDirty();
                                break;
                            }
                            case ComponentPropertyType::EntityReference:
                            {
                                const EntityId current = property.get(*component).get<EntityId>();
                                const Entity* currentTarget = activeScene.FindEntity(current);
                                const std::string preview = currentTarget == nullptr
                                    ? "None" : currentTarget->GetName() + " (#" + std::to_string(current) + ")";
                                if (ImGui::BeginCombo(property.displayName.c_str(), preview.c_str()))
                                {
                                    if (ImGui::Selectable("None", current == 0) &&
                                        property.set(*component, nlohmann::json(std::uint64_t{0})))
                                        MarkSceneDirty();
                                    for (const EntityId candidateId : activeScene.GetEntityIds())
                                    {
                                        if (candidateId == entity->GetId()) continue;
                                        const Entity* candidate = activeScene.FindEntity(candidateId);
                                        if (candidate == nullptr) continue;
                                        const std::string label = candidate->GetName() +
                                            " (#" + std::to_string(candidateId) + ")";
                                        const bool selected = candidateId == current;
                                        if (ImGui::Selectable(label.c_str(), selected) &&
                                            property.set(*component, nlohmann::json(candidateId)))
                                            MarkSceneDirty();
                                        if (selected) ImGui::SetItemDefaultFocus();
                                    }
                                    ImGui::EndCombo();
                                }
                                break;
                            }
                            case ComponentPropertyType::Enumeration:
                            {
                                const int current = property.get(*component).get<int>();
                                const char* preview = "Unknown";
                                for (const auto& option : property.enumValues)
                                    if (option.first == current) preview = option.second.c_str();
                                if (ImGui::BeginCombo(property.displayName.c_str(), preview))
                                {
                                    for (const auto& option : property.enumValues)
                                    {
                                        const bool selected = option.first == current;
                                        if (ImGui::Selectable(option.second.c_str(), selected) &&
                                            property.set(*component, nlohmann::json(option.first)))
                                            MarkSceneDirty();
                                        if (selected) ImGui::SetItemDefaultFocus();
                                    }
                                    ImGui::EndCombo();
                                }
                                break;
                            }
                            }
                            ImGui::PopID();
                        }
                    }
                    if (ImGui::SmallButton("Remove Component"))
                    {
                        (void)entity->RemoveComponent(component.get());
                        MarkSceneDirty();
                        ImGui::TreePop();
                        ImGui::PopID();
                        break;
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (auto* colliderSet = entity->GetComponent<ColliderSet2D>())
            {
                ImGui::SeparatorText("Collider Set 2D");
                if (ImGui::SmallButton("Add Box Collider"))
                {
                    (void)colliderSet->AddBoxCollider();
                    MarkSceneDirty();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Add Circle Collider"))
                {
                    (void)colliderSet->AddCircleCollider();
                    MarkSceneDirty();
                }

                std::optional<Collider2D> pendingDuplicate;
                ColliderId pendingRemoval = 0;
                for (std::size_t index = 0; index < colliderSet->colliders.size(); ++index)
                {
                    Collider2D& collider = colliderSet->colliders[index];
                    ImGui::PushID(static_cast<int>(index));
                    const char* shapeName = collider.shape == ColliderShape2D::Box ? "Box" : "Circle";
                    char colliderLabel[64]{};
                    std::snprintf(colliderLabel, sizeof(colliderLabel),
                        "Collider %zu (%s)", index + 1, shapeName);
                    if (ImGui::TreeNode(colliderLabel))
                    {
                        ImGui::Text("Stable ID: %llu",
                            static_cast<unsigned long long>(collider.id));
                        if (collider.shape == ColliderShape2D::Box)
                        {
                            if (ImGui::DragFloat2("Half Extents", &collider.halfExtents.x,
                                0.1F, 0.001F, 100000.0F)) MarkSceneDirty();
                            if (ImGui::DragFloat("Rotation (rad)", &collider.rotation,
                                0.01F)) MarkSceneDirty();
                        }
                        else
                        {
                            if (ImGui::DragFloat("Radius", &collider.radius,
                                0.1F, 0.001F, 100000.0F)) MarkSceneDirty();
                        }
                        if (ImGui::DragFloat2("Offset", &collider.offset.x, 0.1F))
                            MarkSceneDirty();
                        if (ImGui::DragFloat("Density", &collider.density, 0.01F,
                            0.0F, 100000.0F)) MarkSceneDirty();
                        if (ImGui::DragFloat("Friction", &collider.friction, 0.01F,
                            0.0F, 100000.0F)) MarkSceneDirty();
                        if (ImGui::DragFloat("Restitution", &collider.restitution, 0.01F,
                            0.0F, 100000.0F)) MarkSceneDirty();
                        if (ImGui::Checkbox("Is Sensor", &collider.isSensor))
                            MarkSceneDirty();

                        int categoryBits = static_cast<int>(collider.categoryBits);
                        if (ImGui::InputInt("Category Bits", &categoryBits))
                        {
                            collider.categoryBits = static_cast<std::uint16_t>(
                                std::clamp(categoryBits, 0, 65535));
                            MarkSceneDirty();
                        }
                        int maskBits = static_cast<int>(collider.maskBits);
                        if (ImGui::InputInt("Mask Bits", &maskBits))
                        {
                            collider.maskBits = static_cast<std::uint16_t>(
                                std::clamp(maskBits, 0, 65535));
                            MarkSceneDirty();
                        }

                        if (ImGui::SmallButton("Duplicate"))
                        {
                            Collider2D copy = collider;
                            copy.id = GenerateColliderId();
                            if (copy.id != 0) pendingDuplicate = std::move(copy);
                            else
                                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                    "Collider ID space exhausted");
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) pendingRemoval = collider.id;
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                    if (pendingDuplicate.has_value() || pendingRemoval != 0) break;
                }

                if (pendingDuplicate.has_value())
                {
                    colliderSet->colliders.push_back(std::move(*pendingDuplicate));
                    MarkSceneDirty();
                }
                if (pendingRemoval != 0 && colliderSet->RemoveCollider(pendingRemoval))
                    MarkSceneDirty();
                if (colliderSet->colliders.empty())
                    ImGui::TextDisabled("No colliders. Add a Box or Circle collider.");
            }

            if (ImGui::Button("Add Component"))
                ImGui::OpenPopup("Add Component Popup");
            if (ImGui::BeginPopup("Add Component Popup"))
            {
                for (const auto& type : m_componentRegistry.GetRegisteredTypes())
                {
                    bool alreadyAdded = false;
                    for (const auto& component : entity->GetComponents())
                    {
                        const auto* existing = m_componentRegistry.Find(*component);
                        if (existing && existing->id == type.id) alreadyAdded = true;
                    }
                    ImGui::BeginDisabled(alreadyAdded);
                    if (ImGui::MenuItem(type.displayName.c_str()))
                    {
                        auto component = m_componentRegistry.Create(type.id);
                        if (component)
                        {
                            entity->AddComponent(std::move(component));
                            MarkSceneDirty();
                        }
                    }
                    ImGui::EndDisabled();
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();
            if (ImGui::Button("Delete Entity"))
            {
                const EntityId id = entity->GetId();
                (void)m_scene.DestroyEntity(id);
                m_selectedEntity = 0;
                m_entitySelection.Clear();
                MarkSceneDirty();
            }
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
}

void EditorApplication::DrawInputSettings()
{
    ImGui::SetNextWindowPos(ImVec2(1080.0F, 620.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(440.0F, 420.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Input Settings"))
    {
        ImGui::TextDisabled("Play Mode keyboard actions");
        if (!m_rebindAction.empty())
        {
            ImGui::TextColored(ImVec4(1.0F, 0.78F, 0.25F, 1.0F),
                "Press a key for %s (Esc to cancel)", m_rebindAction.c_str());
        }

        const auto drawActionRow = [this](const std::string& action)
        {
            ImGui::PushID(action.c_str());
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(action.c_str());
            ImGui::TableSetColumnIndex(1);
            const auto& bindings = m_inputMap.GetBindings(action);
            if (bindings.empty())
            {
                ImGui::TextDisabled("Unbound");
            }
            else
            {
                for (std::size_t i = 0; i < bindings.size(); ++i)
                {
                    if (i > 0) ImGui::SameLine();
                    const char* keyName = SDL_GetScancodeName(bindings[i]);
                    ImGui::Text("%s", keyName != nullptr && keyName[0] != '\0' ? keyName : "Unknown");
                }
            }
            ImGui::TableSetColumnIndex(2);
            if (ImGui::SmallButton("Rebind"))
            {
                m_rebindAction = action;
                m_replaceActionOnRebind = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("+Key"))
            {
                m_rebindAction = action;
                m_replaceActionOnRebind = false;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear"))
            {
                m_inputMap.ClearAction(action);
                if (!SaveInputSettings())
                {
                    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                        "Could not save input settings");
                }
            }
            ImGui::PopID();
        };

        if (ImGui::BeginTable("InputActionBindings", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 100.0F);
            ImGui::TableSetupColumn("Keys", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthFixed, 150.0F);
            ImGui::TableHeadersRow();

            drawActionRow("MoveLeft");
            drawActionRow("MoveRight");
            drawActionRow("MoveUp");
            drawActionRow("MoveDown");
            for (const std::string& action : m_customInputActions)
            {
                drawActionRow(action);
            }
            ImGui::EndTable();
        }

        ImGui::SeparatorText("Create action");
        ImGui::SetNextItemWidth(210.0F);
        ImGui::InputText("New action", m_newInputActionName, sizeof(m_newInputActionName));
        ImGui::SameLine();
        if (ImGui::Button("Add + Bind"))
        {
            const std::string action = m_newInputActionName;
            if (action.empty() || IsDefaultInputAction(action) ||
                std::find(m_customInputActions.begin(), m_customInputActions.end(), action)
                    != m_customInputActions.end())
            {
                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                    "Action name is empty or already exists");
            }
            else
            {
                m_customInputActions.push_back(action);
                (void)m_inputMap.CreateAction(action);
                m_selectedCustomInputAction = action;
                if (SaveInputSettings())
                {
                    m_rebindAction = action;
                    m_replaceActionOnRebind = false;
                    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                        "Press a key to bind action: %.170s", action.c_str());
                    m_newInputActionName[0] = '\0';
                }
            }
        }

        ImGui::SeparatorText("Manage custom action");
        if (m_customInputActions.empty())
        {
            ImGui::TextDisabled("No custom actions yet.");
        }
        else
        {
            if (std::find(m_customInputActions.begin(), m_customInputActions.end(),
                m_selectedCustomInputAction) == m_customInputActions.end())
            {
                m_selectedCustomInputAction = m_customInputActions.front();
                std::snprintf(m_renamedInputActionName, sizeof(m_renamedInputActionName),
                    "%s", m_selectedCustomInputAction.c_str());
            }
            if (ImGui::BeginCombo("Action", m_selectedCustomInputAction.c_str()))
            {
                for (const std::string& action : m_customInputActions)
                {
                    const bool selected = action == m_selectedCustomInputAction;
                    if (ImGui::Selectable(action.c_str(), selected))
                    {
                        m_selectedCustomInputAction = action;
                        std::snprintf(m_renamedInputActionName,
                            sizeof(m_renamedInputActionName), "%s", action.c_str());
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SetNextItemWidth(210.0F);
            ImGui::InputText("New name", m_renamedInputActionName,
                sizeof(m_renamedInputActionName));
            ImGui::SameLine();
            if (ImGui::Button("Rename"))
            {
                const std::string newName = m_renamedInputActionName;
                if (newName.empty() || IsDefaultInputAction(newName) ||
                    std::find(m_customInputActions.begin(), m_customInputActions.end(), newName)
                        != m_customInputActions.end())
                {
                    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                        "New action name is empty or already exists");
                }
                else if (m_inputMap.RenameAction(m_selectedCustomInputAction, newName))
                {
                    const std::string oldName = m_selectedCustomInputAction;
                    for (std::string& action : m_customInputActions)
                    {
                        if (action == oldName) action = newName;
                    }
                    if (m_rebindAction == oldName) m_rebindAction = newName;
                    m_selectedCustomInputAction = newName;
                    (void)SaveInputSettings();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete"))
            {
                const std::string removed = m_selectedCustomInputAction;
                m_inputMap.ClearAction(removed);
                m_customInputActions.erase(
                    std::remove(m_customInputActions.begin(), m_customInputActions.end(), removed),
                    m_customInputActions.end());
                if (m_rebindAction == removed) m_rebindAction.clear();
                m_selectedCustomInputAction = m_customInputActions.empty()
                    ? std::string{} : m_customInputActions.front();
                if (!m_selectedCustomInputAction.empty())
                {
                    std::snprintf(m_renamedInputActionName,
                        sizeof(m_renamedInputActionName), "%s",
                        m_selectedCustomInputAction.c_str());
                }
                (void)SaveInputSettings();
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Reset All to Defaults"))
        {
            m_inputMap = InputMap::CreateDefaultMovementMap();
            m_customInputActions.clear();
            m_selectedCustomInputAction.clear();
            m_rebindAction.clear();
            (void)SaveInputSettings();
            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                "Input actions restored to default bindings");
        }
        ImGui::TextDisabled("Keys cannot be shared by different actions. Settings save to the project.");
    }
    ImGui::End();
}

void EditorApplication::DrawProjectSettings()
{
    ImGui::SetNextWindowPos(ImVec2(0.0F, 620.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280.0F, 260.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Project Settings"))
    {
        ImGui::BeginDisabled(m_isPlaying);
        ImGui::SetNextItemWidth(-1.0F);
        ImGui::InputText("Project Name", m_projectName, sizeof(m_projectName));
        ImGui::SetNextItemWidth(-1.0F);
        ImGui::InputText("Asset Directory", m_assetDirectory, sizeof(m_assetDirectory));
        ImGui::SetNextItemWidth(-1.0F);
        ImGui::InputText("Startup Scene", m_startupScenePath, sizeof(m_startupScenePath));
        ImGui::TextDisabled("Scene path is relative to the .emberproject file.");
        if (ImGui::Button("Save Project Settings"))
        {
            if (SaveProjectSettings())
            {
                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                    "Project settings saved");
            }
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
}

void EditorApplication::DrawAssetBrowser()
{
    ImGui::SetNextWindowPos(ImVec2(280.0F, 650.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800.0F, 220.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Asset Browser"))
    {
        ImGui::Text("Asset root: %s", m_assetRegistry.GetDirectory().GetRoot().string().c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) RefreshAssets();
        ImGui::SameLine();
        ImGui::TextDisabled("%zu indexed assets | %zu diagnostics",
            m_assetRegistry.GetAssetCount(), m_assetRegistry.GetDiagnostics().size());

        ImGui::InputText("Search", m_assetSearch, sizeof(m_assetSearch));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0F);
        ImGui::Combo("Type", &m_assetTypeFilter, "All\0Textures\0Audio\0Scenes\0Scripts\0");

        std::vector<const AssetMetadata*> assets;
        assets.reserve(m_assetRegistry.GetAssets().size());
        for (const auto& [assetId, metadata] : m_assetRegistry.GetAssets())
        {
            (void)assetId;
            if (!MatchesAssetType(metadata.type, m_assetTypeFilter)) continue;
            const std::string source = metadata.source.generic_string();
            if (m_assetSearch[0] != '\0' &&
                metadata.assetId.find(m_assetSearch) == std::string::npos &&
                source.find(m_assetSearch) == std::string::npos) continue;
            assets.push_back(&metadata);
        }
        std::sort(assets.begin(), assets.end(), [](const AssetMetadata* left, const AssetMetadata* right)
        {
            return left->source.generic_string() < right->source.generic_string();
        });

        if (ImGui::BeginTable("AssetList", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
            ImVec2(0.0F, 105.0F)))
        {
            ImGui::TableSetupColumn("Asset", ImGuiTableColumnFlags_WidthStretch, 0.45F);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 85.0F);
            ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch, 0.55F);
            ImGui::TableHeadersRow();

            for (const AssetMetadata* metadata : assets)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                const bool selected = m_selectedAssetId == metadata->assetId;
                const std::string label = metadata->assetId + "##asset";
                if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    if (m_selectedAssetId != metadata->assetId) StopAudioPreview();
                    m_selectedAssetId = metadata->assetId;
                    LoadSelectedTextureSettings();
                }
                ImGui::TableSetColumnIndex(1);
                std::string typeLabel = AssetTypeName(metadata->type);
                if (metadata->type == AssetType::Script)
                    typeLabel += metadata->scriptLanguage.empty() ? " / Unassigned" : " / " + metadata->scriptLanguage;
                ImGui::TextUnformatted(typeLabel.c_str());
                ImGui::TableSetColumnIndex(2);
                const std::string source = metadata->source.generic_string();
                ImGui::TextUnformatted(source.c_str());
            }
            ImGui::EndTable();
        }

        const AssetMetadata* selected = m_assetRegistry.Find(m_selectedAssetId);
        if (selected != nullptr)
        {
            ImGui::Separator();
            ImGui::Text("Selected: %s (%s)", selected->assetId.c_str(), AssetTypeName(selected->type));
            ImGui::Text("Source: %s", selected->source.generic_string().c_str());
            if (selected->type == AssetType::Script)
            {
                ImGui::Text("Language: %s", selected->scriptLanguage.empty()
                    ? "Unassigned" : selected->scriptLanguage.c_str());
                ImGui::TextDisabled("%s", selected->scriptLanguage == "lua"
                    ? "Runtime backend: Lua 5.5.1 (available in Play Mode)"
                    : "Runtime backend: no matching backend is currently available");
            }

            if (selected->type == AssetType::Script)
            {
                ImGui::SeparatorText("Script Actions");
                ImGui::BeginDisabled(m_isPlaying);
                if (ImGui::Button("Open in Script Source Editor"))
                {
                    OpenSelectedScriptSource();
                }
                ImGui::EndDisabled();
                ImGui::TextDisabled("Edit text-based Script assets directly in the Editor (64 KiB limit).");
            }

            if (selected->type == AssetType::Scene)
            {
                ImGui::SeparatorText("Scene Actions");
                const std::filesystem::path projectRelativeScene =
                    m_projectConfiguration.assetDirectory / selected->source;
                if (ImGui::Button("Open Selected Scene"))
                {
                    RequestLoadScene(projectRelativeScene);
                }
                ImGui::SameLine();
                if (ImGui::Button("Set as Startup Scene"))
                {
                    const std::string path = PathToUtf8(projectRelativeScene);
                    std::snprintf(m_startupScenePath, sizeof(m_startupScenePath),
                        "%s", path.c_str());
                    if (SaveProjectSettings())
                    {
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "Startup scene set: %.190s", path.c_str());
                    }
                }
            }
            else if (selected->type == AssetType::Texture)
            {
                ImGui::SeparatorText("Texture Import Settings");
                ImGui::SetNextItemWidth(150.0F);
                ImGui::Combo("Filter", &m_textureFilter, "Nearest\0Linear\0");
                ImGui::Checkbox("Premultiply Alpha", &m_premultiplyAlpha);
                ImGui::SameLine();
                if (ImGui::Button("Apply Settings"))
                {
                    TextureImportSettings settings;
                    settings.filter = m_textureFilter == 1 ? TextureFilter::Linear : TextureFilter::Nearest;
                    settings.premultiplyAlpha = m_premultiplyAlpha;
                    std::string error;
                    if (m_assetImportService.UpdateTextureSettings(m_selectedAssetId, settings, &error))
                    {
                        ClearPreviewTextures();
                        (void)m_assetRegistry.Scan();
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "Saved import settings for %.180s", m_selectedAssetId.c_str());
                    }
                    else
                    {
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "Import settings failed: %.200s", error.c_str());
                    }
                }
                ImGui::TextDisabled("Changes are saved to the .emberasset sidecar. Reload runtime assets to apply.");
            }
            else if (selected->type == AssetType::Audio)
            {
                ImGui::SeparatorText("Audio Preview");
                const bool previewLoaded = m_audioPreviewSound &&
                    m_audioPreviewAssetId == selected->assetId;
                const bool previewPlaying = previewLoaded &&
                    m_audio.IsPlaying(m_audioPreviewSound);
                ImGui::BeginDisabled(m_isPlaying || !m_audio.IsInitialized());
                if (ImGui::Button(previewPlaying ? "Stop Preview" : "Play Preview"))
                {
                    if (previewPlaying)
                    {
                        if (m_audio.Stop(m_audioPreviewSound))
                            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                "Audio preview stopped");
                        else
                            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                "Could not stop audio preview");
                    }
                    else
                    {
                        if (!previewLoaded)
                        {
                            StopAudioPreview();
                            const auto path = m_assetRegistry.GetDirectory().Resolve(selected->source);
                            if (!path)
                            {
                                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                    "Audio preview path is invalid or outside the asset directory");
                            }
                            else
                            {
                                m_audioPreviewSound = m_audio.LoadSound(*path, false);
                                if (m_audioPreviewSound)
                                    m_audioPreviewAssetId = selected->assetId;
                                else
                                    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                        "Could not load audio preview: %.170s", selected->source.string().c_str());
                            }
                        }

                        if (m_audioPreviewSound && m_audio.Play(m_audioPreviewSound))
                        {
                            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                "Playing audio preview: %.180s", selected->assetId.c_str());
                        }
                        else if (m_audioPreviewSound)
                        {
                            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                                "Could not start audio preview");
                        }
                    }
                }
                ImGui::EndDisabled();
                if (!m_audio.IsInitialized())
                    ImGui::TextDisabled("Audio device is unavailable; preview is disabled.");
                else
                    ImGui::TextDisabled("Preview plays once through the Editor audio device.");
            }
            else
            {
                ImGui::TextDisabled("No additional actions are available for this asset type.");
            }
        }
        else if (!m_selectedAssetId.empty())
        {
            ImGui::TextDisabled("The previously selected asset is no longer indexed.");
        }


        ImGui::SeparatorText("Import Script Source");
        ImGui::SetNextItemWidth(360.0F);
        ImGui::InputText("Asset-relative source path", m_scriptImportSource, sizeof(m_scriptImportSource));
        ImGui::SameLine();
        ImGui::BeginDisabled(m_isPlaying || m_scriptImportSource[0] == '\0');
        if (ImGui::Button("Import Script"))
        {
            std::string error;
            if (m_assetImportService.ImportScript(m_scriptImportSource, &error))
            {
                (void)m_assetRegistry.Scan();
                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                    "Imported Script asset: %.180s", m_scriptImportSource);
                m_scriptImportSource[0] = '\0';
            }
            else
            {
                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                    "Script import failed: %.190s", error.c_str());
            }
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Use a path relative to the asset root. .lua files are tagged for Lua 5.5.1; other text formats remain unassigned until a matching backend is available.");

        if (ImGui::CollapsingHeader("Import / Scan Diagnostics"))
        {
            const auto& diagnostics = m_assetRegistry.GetDiagnostics();
            if (diagnostics.empty())
            {
                ImGui::TextDisabled("No asset diagnostics.");
            }
            else
            {
                for (const auto& diagnostic : diagnostics)
                {
                    ImGui::PushID(&diagnostic);
                    ImGui::TextWrapped("%s: %s",
                        diagnostic.path.string().c_str(), diagnostic.message.c_str());
                    ImGui::PopID();
                }
            }
        }

        ImGui::Separator();
        ImGui::SeparatorText("Scene Manager");
        ImGui::BeginDisabled(m_isPlaying);
        ImGui::SetNextItemWidth(330.0F);
        ImGui::InputText("Project-relative scene path", m_scenePath, sizeof(m_scenePath));
        if (ImGui::Button("New Scene"))
        {
            RequestNewScene(m_projectConfiguration.assetDirectory / "scenes/untitled.emberscene");
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Scene")) LoadScene();
        ImGui::SameLine();
        if (ImGui::Button("Save Scene")) (void)SaveScene();
        ImGui::SameLine();
        if (ImGui::Button("Set Current as Startup"))
        {
            std::snprintf(m_startupScenePath, sizeof(m_startupScenePath),
                "%s", PathToUtf8(m_currentScenePath).c_str());
            if (SaveProjectSettings())
            {
                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                    "Current scene saved as project startup scene");
            }
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Load path is project-relative. Save Scene writes to the current scene path.");
        ImGui::TextDisabled(m_sceneDirty ? "Unsaved scene changes (*)" : "Scene saved");
    }
    ImGui::End();
}

SDL_Texture* EditorApplication::GetPreviewTexture(const AssetMetadata& metadata)
{
    const auto cached = m_previewTextures.find(metadata.assetId);
    if (cached != m_previewTextures.end())
    {
        return cached->second;
    }

    const auto resolvedPath = m_assetRegistry.GetDirectory().Resolve(metadata.source);
    if (!resolvedPath)
    {
        return nullptr;
    }

    const std::string utf8Path = PathToUtf8(*resolvedPath);
    SDL_Surface* surface = IMG_Load(utf8Path.c_str());
    if (surface == nullptr)
    {
        return nullptr;
    }

    SDL_Surface* uploadSurface = surface;
    if (metadata.texture.premultiplyAlpha)
    {
        uploadSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (uploadSurface == nullptr)
        {
            SDL_DestroySurface(surface);
            return nullptr;
        }

        if (SDL_MUSTLOCK(uploadSurface) && !SDL_LockSurface(uploadSurface))
        {
            SDL_DestroySurface(uploadSurface);
            SDL_DestroySurface(surface);
            return nullptr;
        }

        for (int y = 0; y < uploadSurface->h; ++y)
        {
            auto* row = static_cast<std::uint8_t*>(uploadSurface->pixels)
                      + static_cast<std::ptrdiff_t>(y) * uploadSurface->pitch;
            for (int x = 0; x < uploadSurface->w; ++x)
            {
                std::uint8_t* pixel = row + static_cast<std::ptrdiff_t>(x) * 4;
                const unsigned int alpha = pixel[3];
                pixel[0] = static_cast<std::uint8_t>((static_cast<unsigned int>(pixel[0]) * alpha + 127U) / 255U);
                pixel[1] = static_cast<std::uint8_t>((static_cast<unsigned int>(pixel[1]) * alpha + 127U) / 255U);
                pixel[2] = static_cast<std::uint8_t>((static_cast<unsigned int>(pixel[2]) * alpha + 127U) / 255U);
            }
        }

        if (SDL_MUSTLOCK(uploadSurface))
        {
            SDL_UnlockSurface(uploadSurface);
        }
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, uploadSurface);
    if (uploadSurface != surface)
    {
        SDL_DestroySurface(uploadSurface);
    }
    SDL_DestroySurface(surface);

    if (texture == nullptr)
    {
        return nullptr;
    }

    const SDL_ScaleMode scaleMode = metadata.texture.filter == TextureFilter::Linear
        ? SDL_SCALEMODE_LINEAR
        : SDL_SCALEMODE_NEAREST;
    if (!SDL_SetTextureScaleMode(texture, scaleMode))
    {
        SDL_DestroyTexture(texture);
        return nullptr;
    }

    if (metadata.texture.premultiplyAlpha &&
        !SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND_PREMULTIPLIED))
    {
        SDL_DestroyTexture(texture);
        return nullptr;
    }

    m_previewTextures.emplace(metadata.assetId, texture);
    return texture;
}

void EditorApplication::StopAudioPreview()
{
    if (m_audioPreviewSound && m_audio.IsInitialized())
    {
        (void)m_audio.Stop(m_audioPreviewSound);
        (void)m_audio.UnloadSound(m_audioPreviewSound);
    }
    m_audioPreviewSound = {};
    m_audioPreviewAssetId.clear();
}

void EditorApplication::ClearPreviewTextures()
{
    for (const auto& [assetId, texture] : m_previewTextures)
    {
        (void)assetId;
        if (texture != nullptr)
        {
            SDL_DestroyTexture(texture);
        }
    }
    m_previewTextures.clear();
}

void EditorApplication::DrawViewport()
{
    ImGui::SetNextWindowPos(ImVec2(280.0F, 24.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800.0F, 620.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("2D Viewport"))
    {
        Scene& activeScene = GetActiveScene();
        if (m_isPlaying)
        {
            ImGui::TextColored(ImVec4(0.35F, 0.9F, 0.55F, 1.0F),
                "PLAY MODE - isolated scene preview | UI input active");
        }
        ImGui::TextDisabled("Middle-drag: pan | Wheel: zoom | Click: select");
        ImGui::SameLine();
        if (ImGui::SmallButton(m_gizmoMode == 0 ? "[Move]" : "Move")) m_gizmoMode = 0;
        ImGui::SameLine();
        if (ImGui::SmallButton(m_gizmoMode == 1 ? "[Rotate]" : "Rotate")) m_gizmoMode = 1;
        ImGui::SameLine();
        if (ImGui::SmallButton(m_gizmoMode == 2 ? "[Scale]" : "Scale")) m_gizmoMode = 2;
        ImGui::SameLine();
        ImGui::Checkbox("Local", &m_gizmoLocalSpace);
        ImGui::SameLine();
        ImGui::Checkbox("Snap", &m_snapTransforms);
        if (m_snapTransforms)
        {
            ImGui::SameLine();
            if (m_gizmoMode == 0)
            {
                ImGui::SetNextItemWidth(72.0F);
                ImGui::DragFloat("##PositionSnap", &m_positionSnap, 1.0F, 0.1F, 256.0F, "%.1f");
                ImGui::SameLine();
                ImGui::TextDisabled("units");
            }
            else if (m_gizmoMode == 1)
            {
                ImGui::SetNextItemWidth(72.0F);
                ImGui::DragFloat("##RotationSnap", &m_rotationSnapDegrees, 1.0F, 1.0F, 180.0F, "%.0f");
                ImGui::SameLine();
                ImGui::TextDisabled("deg");
            }
            else
            {
                ImGui::SetNextItemWidth(72.0F);
                ImGui::DragFloat("##ScaleSnap", &m_scaleSnap, 0.01F, 0.01F, 2.0F, "%.2f");
                ImGui::SameLine();
                ImGui::TextDisabled("scale");
            }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset View"))
        {
            m_viewportCamera.SetPosition({0.0F, 0.0F});
            m_viewportCamera.SetZoom(1.0F);
            m_viewportCamera.SetRotation(0.0F);
        }
        ImGui::SameLine();
        ImGui::Checkbox("Physics", &m_showPhysicsDebugOverlay);
        ImGui::SameLine();
        ImGui::Text("Zoom: %.0f%%", m_viewportCamera.GetZoom() * 100.0F);

        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        canvasSize.x = std::max(canvasSize.x, 64.0F);
        canvasSize.y = std::max(canvasSize.y, 64.0F);
        ImGui::InvisibleButton("##EMBERViewportCanvas", canvasSize,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);

        const ImVec2 canvasMin = ImGui::GetItemRectMin();
        const ImVec2 canvasMax = ImGui::GetItemRectMax();
        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 viewportSize{canvasMax.x - canvasMin.x, canvasMax.y - canvasMin.y};
        m_viewportCamera.SetViewportSize(viewportSize.x, viewportSize.y);

        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            const ImVec2 delta = ImGui::GetIO().MouseDelta;
            const Vector2 cameraPosition = m_viewportCamera.GetPosition();
            const float zoom = m_viewportCamera.GetZoom();
            m_viewportCamera.SetPosition({
                cameraPosition.x - delta.x / zoom,
                cameraPosition.y - delta.y / zoom
            });
        }

        if (hovered && ImGui::GetIO().MouseWheel != 0.0F)
        {
            const float nextZoom = std::clamp(
                m_viewportCamera.GetZoom() * (ImGui::GetIO().MouseWheel > 0.0F ? 1.1F : 0.9F),
                0.1F, 8.0F);
            m_viewportCamera.SetZoom(nextZoom);
        }

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const auto screenToWorld = [&](const ImVec2& screenPosition)
        {
            const Vector2 screenOffset{
                screenPosition.x - canvasMin.x - viewportSize.x * 0.5F,
                screenPosition.y - canvasMin.y - viewportSize.y * 0.5F
            };
            const float inverseZoom = 1.0F / m_viewportCamera.GetZoom();
            const float cosine = std::cos(m_viewportCamera.GetRotation());
            const float sine = std::sin(m_viewportCamera.GetRotation());
            const Vector2 worldOffset{
                (screenOffset.x * cosine - screenOffset.y * sine) * inverseZoom,
                (screenOffset.x * sine + screenOffset.y * cosine) * inverseZoom
            };
            return m_viewportCamera.GetPosition() + worldOffset;
        };
        const Vector2 worldMouse = screenToWorld(mouse);

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            EntityId gizmoEntity = 0;
            Vector2 gizmoAxis{};
            bool rotationHandle = false;
            bool scaleHandle = false;
            float bestHandleDistance = 12.0F;
            if (!m_isPlaying && m_selectedEntity != 0)
            {
                if (const Entity* selected = activeScene.FindEntity(m_selectedEntity))
                {
                    const auto transform = activeScene.GetWorldTransform(selected->GetId()).value_or(selected->GetTransform());
                    const auto& render = selected->GetRenderComponent();
                    const Vector2 origin{
                        transform.position.x + render.size.x * transform.scale.x * 0.5F,
                        transform.position.y + render.size.y * transform.scale.y * 0.5F
                    };
                    const Vector2 originScreenWorld = m_viewportCamera.WorldToScreen(origin);
                    const ImVec2 originScreen{
                        canvasMin.x + originScreenWorld.x,
                        canvasMin.y + originScreenWorld.y
                    };
                    const float axisLength = 80.0F / m_viewportCamera.GetZoom();
                    const float axisCosine = m_gizmoLocalSpace ? std::cos(transform.rotation) : 1.0F;
                    const float axisSine = m_gizmoLocalSpace ? std::sin(transform.rotation) : 0.0F;
                    const Vector2 axes[2]{
                        {axisCosine, axisSine}, {-axisSine, axisCosine}
                    };
                    const Vector2 xEndWorld = origin + axes[0] * axisLength;
                    const Vector2 yEndWorld = origin + axes[1] * axisLength;
                    const Vector2 xScreenWorld = m_viewportCamera.WorldToScreen(xEndWorld);
                    const Vector2 yScreenWorld = m_viewportCamera.WorldToScreen(yEndWorld);
                    const ImVec2 ends[2]{
                        ImVec2{canvasMin.x + xScreenWorld.x, canvasMin.y + xScreenWorld.y},
                        ImVec2{canvasMin.x + yScreenWorld.x, canvasMin.y + yScreenWorld.y}
                    };
                    const ImVec2 start = ImGui::GetIO().MousePos;
                    if (m_gizmoMode == 1)
                    {
                        const float dx = start.x - originScreen.x;
                        const float dy = start.y - originScreen.y;
                        const float ringDistance = std::abs(std::sqrt(dx * dx + dy * dy) - 58.0F);
                        if (ringDistance < bestHandleDistance)
                        {
                            bestHandleDistance = ringDistance;
                            gizmoEntity = selected->GetId();
                            rotationHandle = true;
                        }
                    }
                    else if (m_gizmoMode == 2)
                    {
                        for (int i = 0; i < 2; ++i)
                        {
                            const float dx = start.x - ends[i].x;
                            const float dy = start.y - ends[i].y;
                            const float distance = std::sqrt(dx * dx + dy * dy);
                            if (distance < bestHandleDistance)
                            {
                                bestHandleDistance = distance;
                                gizmoEntity = selected->GetId();
                                gizmoAxis = axes[i];
                                m_dragGizmoAxisIndex = i;
                                scaleHandle = true;
                            }
                        }
                    }
                    else
                    {
                        const ImVec2 points[2]{ends[0], ends[1]};
                        for (int i = 0; i < 2; ++i)
                        {
                            const ImVec2 segment{points[i].x - originScreen.x, points[i].y - originScreen.y};
                            const ImVec2 relative{start.x - originScreen.x, start.y - originScreen.y};
                            const float lengthSquared = segment.x * segment.x + segment.y * segment.y;
                            const float t = lengthSquared > 0.0F
                                ? std::clamp((relative.x * segment.x + relative.y * segment.y) / lengthSquared, 0.0F, 1.0F)
                                : 0.0F;
                            const float dx = start.x - (originScreen.x + segment.x * t);
                            const float dy = start.y - (originScreen.y + segment.y * t);
                            const float distance = std::sqrt(dx * dx + dy * dy);
                            if (distance < bestHandleDistance)
                            {
                                bestHandleDistance = distance;
                                gizmoEntity = selected->GetId();
                                gizmoAxis = axes[i];
                                m_dragGizmoAxisIndex = i;
                            }
                        }
                    }
                }
            }

            m_dragEntityId = 0;
            m_draggingGizmoAxis = false;
            m_draggingRotationGizmo = false;
            m_draggingScaleGizmo = false;
            if (gizmoEntity != 0)
            {
                m_dragEntityId = gizmoEntity;
                m_draggingRotationGizmo = rotationHandle;
                m_draggingScaleGizmo = scaleHandle;
                m_draggingGizmoAxis = !rotationHandle && !scaleHandle;
                m_dragGizmoAxis = gizmoAxis;
                m_dragStartMouseWorld = worldMouse;
                if (const Entity* entity = activeScene.FindEntity(gizmoEntity))
                {
                    const auto transform = activeScene.GetWorldTransform(entity->GetId()).value_or(entity->GetTransform());
                    m_dragStartEntityPosition = transform.position;
                    m_dragStartEntityRotation = transform.rotation;
                    m_dragStartEntityScale = transform.scale;
                    const auto& render = entity->GetRenderComponent();
                    const Vector2 pivot{
                        transform.position.x + render.size.x * transform.scale.x * 0.5F,
                        transform.position.y + render.size.y * transform.scale.y * 0.5F
                    };
                    const Vector2 mouseOffset = worldMouse - pivot;
                    m_dragStartMouseAngle = std::atan2(mouseOffset.y, mouseOffset.x);
                }
            }
            else
            {
                EntityId hitEntity = 0;
                activeScene.ForEachEntity([&](const Entity& entity)
                {
                    const auto& render = entity.GetRenderComponent();
                    if (!entity.IsActive() || !render.visible) return;

                    const auto transform = activeScene.GetWorldTransform(entity.GetId()).value_or(entity.GetTransform());
                    const Vector2 scaledSize{
                        render.size.x * transform.scale.x,
                        render.size.y * transform.scale.y
                    };
                    const Vector2 center{
                        transform.position.x + scaledSize.x * 0.5F,
                        transform.position.y + scaledSize.y * 0.5F
                    };
                    const float cosine = std::cos(transform.rotation);
                    const float sine = std::sin(transform.rotation);
                    const Vector2 offset = worldMouse - center;
                    const Vector2 localPoint{
                        offset.x * cosine + offset.y * sine,
                        -offset.x * sine + offset.y * cosine
                    };
                    if (std::abs(localPoint.x) <= std::abs(scaledSize.x) * 0.5F &&
                        std::abs(localPoint.y) <= std::abs(scaledSize.y) * 0.5F)
                        hitEntity = entity.GetId();
                });
                if (hitEntity != 0) m_entitySelection.SelectOnly(hitEntity);
                else m_entitySelection.Clear();
                m_selectedEntity = m_entitySelection.Primary();
                if (!m_isPlaying && hitEntity != 0)
                {
                    if (Entity* entity = activeScene.FindEntity(hitEntity))
                    {
                        m_dragEntityId = hitEntity;
                        m_dragStartMouseWorld = worldMouse;
                        m_dragStartEntityPosition = activeScene.GetWorldTransform(entity->GetId()).value_or(entity->GetTransform()).position;
                    }
                }
            }
        }

        if (!m_isPlaying && m_dragEntityId != 0 &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            if (Entity* entity = activeScene.FindEntity(m_dragEntityId))
            {
                const Vector2 movement = worldMouse - m_dragStartMouseWorld;
                Transform2D transform = activeScene.GetWorldTransform(entity->GetId()).value_or(entity->GetTransform());
                if (m_draggingRotationGizmo)
                {
                    const auto& render = entity->GetRenderComponent();
                    const Vector2 pivot{
                        transform.position.x + render.size.x * transform.scale.x * 0.5F,
                        transform.position.y + render.size.y * transform.scale.y * 0.5F
                    };
                    const Vector2 mouseOffset = worldMouse - pivot;
                    const float currentAngle = std::atan2(mouseOffset.y, mouseOffset.x);
                    const float angleDelta = std::atan2(
                        std::sin(currentAngle - m_dragStartMouseAngle),
                        std::cos(currentAngle - m_dragStartMouseAngle));
                    const float candidate = m_dragStartEntityRotation + angleDelta;
                    if (m_snapTransforms)
                    {
                        constexpr float radiansPerDegree = 0.017453292519943295F;
                        const float step = m_rotationSnapDegrees * radiansPerDegree;
                        transform.rotation = SnapValue(candidate, step);
                    }
                    else
                    {
                        transform.rotation = candidate;
                    }
                }
                else if (m_draggingScaleGizmo)
                {
                    const auto& render = entity->GetRenderComponent();
                    const float distance = movement.x * m_dragGizmoAxis.x +
                        movement.y * m_dragGizmoAxis.y;
                    const float baseSize = m_dragGizmoAxisIndex == 0 ? render.size.x : render.size.y;
                    if (std::abs(baseSize) > 0.0001F)
                    {
                        const float startScale = m_dragGizmoAxisIndex == 0
                            ? m_dragStartEntityScale.x : m_dragStartEntityScale.y;
                        const float candidate = startScale + distance / baseSize;
                        const float nextScale = std::max(0.01F,
                            m_snapTransforms ? SnapValue(candidate, m_scaleSnap) : candidate);
                        if (m_dragGizmoAxisIndex == 0) transform.scale.x = nextScale;
                        else transform.scale.y = nextScale;
                    }
                }
                else if (m_draggingGizmoAxis)
                {
                    const float distance = movement.x * m_dragGizmoAxis.x +
                        movement.y * m_dragGizmoAxis.y;
                    transform.position = m_dragStartEntityPosition +
                        Vector2{m_dragGizmoAxis.x * distance, m_dragGizmoAxis.y * distance};
                    if (m_snapTransforms)
                    {
                        transform.position.x = SnapValue(transform.position.x, m_positionSnap);
                        transform.position.y = SnapValue(transform.position.y, m_positionSnap);
                    }
                }
                else
                {
                    transform.position = m_dragStartEntityPosition + movement;
                    if (m_snapTransforms)
                    {
                        transform.position.x = SnapValue(transform.position.x, m_positionSnap);
                        transform.position.y = SnapValue(transform.position.y, m_positionSnap);
                    }
                }

                const bool transformApplied = activeScene.SetWorldTransform(entity->GetId(), transform);
                const auto current = activeScene.GetWorldTransform(entity->GetId()).value_or(transform);
                if (transformApplied && (current.position.x != m_dragStartEntityPosition.x ||
                    current.position.y != m_dragStartEntityPosition.y ||
                    current.rotation != m_dragStartEntityRotation ||
                    current.scale.x != m_dragStartEntityScale.x ||
                    current.scale.y != m_dragStartEntityScale.y))
                {
                    MarkSceneDirty();
                }
            }
            else
            {
                m_dragEntityId = 0;
                m_draggingGizmoAxis = false;
                m_draggingRotationGizmo = false;
                m_draggingScaleGizmo = false;
            }
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_dragEntityId = 0;
            m_draggingGizmoAxis = false;
            m_draggingRotationGizmo = false;
            m_draggingScaleGizmo = false;
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(canvasMin, canvasMax, true);
        drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(25, 29, 38, 255));

        const float zoom = m_viewportCamera.GetZoom();
        const Vector2 cameraPosition = m_viewportCamera.GetPosition();
        float gridStep = 64.0F;
        if (gridStep * zoom < 16.0F)
        {
            gridStep *= std::ceil(16.0F / (gridStep * zoom));
        }
        const float halfWorldWidth = viewportSize.x * 0.5F / zoom;
        const float halfWorldHeight = viewportSize.y * 0.5F / zoom;
        const float left = cameraPosition.x - halfWorldWidth;
        const float right = cameraPosition.x + halfWorldWidth;
        const float top = cameraPosition.y - halfWorldHeight;
        const float bottom = cameraPosition.y + halfWorldHeight;
        const auto toCanvas = [&](const Vector2& world)
        {
            const Vector2 screen = m_viewportCamera.WorldToScreen(world);
            return ImVec2(canvasMin.x + screen.x, canvasMin.y + screen.y);
        };

        const float firstGridX = std::floor(left / gridStep) * gridStep;
        for (float x = firstGridX; x <= right && x - firstGridX < gridStep * 1000.0F; x += gridStep)
        {
            const ImVec2 a = toCanvas({x, top});
            const ImVec2 b = toCanvas({x, bottom});
            drawList->AddLine(a, b, IM_COL32(48, 54, 67, 255));
        }
        const float firstGridY = std::floor(top / gridStep) * gridStep;
        for (float y = firstGridY; y <= bottom && y - firstGridY < gridStep * 1000.0F; y += gridStep)
        {
            const ImVec2 a = toCanvas({left, y});
            const ImVec2 b = toCanvas({right, y});
            drawList->AddLine(a, b, IM_COL32(48, 54, 67, 255));
        }
        drawList->AddLine(toCanvas({0.0F, top}), toCanvas({0.0F, bottom}), IM_COL32(105, 115, 140, 255), 1.5F);
        drawList->AddLine(toCanvas({left, 0.0F}), toCanvas({right, 0.0F}), IM_COL32(105, 115, 140, 255), 1.5F);

        activeScene.ForEachEntity([&](const Entity& entity)
        {
            const auto& render = entity.GetRenderComponent();
            if (!entity.IsActive() || !render.visible) return;

            const auto transform = activeScene.GetWorldTransform(entity.GetId()).value_or(entity.GetTransform());
            const Vector2 scaledSize{
                render.size.x * transform.scale.x,
                render.size.y * transform.scale.y
            };
            const Vector2 center{
                transform.position.x + scaledSize.x * 0.5F,
                transform.position.y + scaledSize.y * 0.5F
            };
            const float cosine = std::cos(transform.rotation);
            const float sine = std::sin(transform.rotation);
            const float halfWidth = scaledSize.x * 0.5F;
            const float halfHeight = scaledSize.y * 0.5F;
            const Vector2 localCorners[4]{
                {-halfWidth, -halfHeight}, {halfWidth, -halfHeight},
                {halfWidth, halfHeight}, {-halfWidth, halfHeight}
            };
            ImVec2 corners[4]{};
            for (int i = 0; i < 4; ++i)
            {
                const Vector2& corner = localCorners[i];
                const Vector2 worldCorner{
                    center.x + corner.x * cosine - corner.y * sine,
                    center.y + corner.x * sine + corner.y * cosine
                };
                corners[i] = toCanvas(worldCorner);
            }

            const ImU32 tint = IM_COL32(render.red, render.green, render.blue, render.alpha);
            SDL_Texture* previewTexture = nullptr;
            if (!render.textureAssetId.empty())
            {
                const AssetMetadata* metadata = m_assetRegistry.Find(render.textureAssetId);
                if (metadata != nullptr && metadata->type == AssetType::Texture)
                {
                    previewTexture = GetPreviewTexture(*metadata);
                }
            }

            if (previewTexture != nullptr)
            {
                const ImTextureID textureId = (ImTextureID)(intptr_t)previewTexture;
                ImVec2 uvTopLeft(0.0F, 0.0F);
                ImVec2 uvTopRight(1.0F, 0.0F);
                ImVec2 uvBottomRight(1.0F, 1.0F);
                ImVec2 uvBottomLeft(0.0F, 1.0F);
                if (const auto* animation = entity.GetComponent<SpriteAnimation2D>();
                    animation != nullptr && animation->IsConfigurationValid())
                {
                    float textureWidth = 0.0F;
                    float textureHeight = 0.0F;
                    if (SDL_GetTextureSize(previewTexture, &textureWidth, &textureHeight))
                    {
                        const int frameColumn = animation->GetCurrentFrame() % animation->columns;
                        const int frameRow = animation->GetCurrentFrame() / animation->columns;
                        const float sourceX = static_cast<float>(frameColumn) *
                            static_cast<float>(animation->frameWidth);
                        const float sourceY = static_cast<float>(frameRow) *
                            static_cast<float>(animation->frameHeight);
                        if (sourceX + animation->frameWidth <= textureWidth &&
                            sourceY + animation->frameHeight <= textureHeight)
                        {
                            const float u0 = sourceX / textureWidth;
                            const float v0 = sourceY / textureHeight;
                            const float u1 = (sourceX + animation->frameWidth) / textureWidth;
                            const float v1 = (sourceY + animation->frameHeight) / textureHeight;
                            uvTopLeft = ImVec2(u0, v0);
                            uvTopRight = ImVec2(u1, v0);
                            uvBottomRight = ImVec2(u1, v1);
                            uvBottomLeft = ImVec2(u0, v1);
                        }
                    }
                }
                drawList->AddImageQuad(
                    textureId,
                    corners[0], corners[1], corners[2], corners[3],
                    uvTopLeft, uvTopRight, uvBottomRight, uvBottomLeft,
                    tint);
            }
            else
            {
                drawList->AddConvexPolyFilled(corners, 4, tint);
                if (!render.textureAssetId.empty())
                {
                    const ImVec2 centerPoint = toCanvas(center);
                    drawList->AddText(centerPoint, IM_COL32(255, 190, 120, 255), "Missing Texture");
                }
            }
            drawList->AddPolyline(corners, 4, IM_COL32(12, 14, 20, 255), ImDrawFlags_Closed, 1.0F);

            if (entity.GetId() == m_selectedEntity)
            {
                drawList->AddPolyline(corners, 4, IM_COL32(255, 210, 80, 255), ImDrawFlags_Closed, 2.0F);
                const ImVec2 labelPosition{corners[0].x, corners[0].y - 18.0F};
                drawList->AddText(labelPosition, IM_COL32(255, 225, 140, 255), entity.GetName().c_str());
            }
        });

        // UI Canvas is a screen-space layer rendered after world entities.
        // The first active, valid Canvas in stable EntityId order defines the
        // reference resolution for all enabled UI panels in this Scene.
        const UICanvas2D* activeCanvas = nullptr;
        activeScene.ForEachEntity([&](const Entity& entity)
        {
            if (activeCanvas != nullptr || !entity.IsActive()) return;
            const auto* canvas = entity.GetComponent<UICanvas2D>();
            if (canvas != nullptr && canvas->enabled && canvas->IsConfigurationValid())
                activeCanvas = canvas;
        });
        if (m_isPlaying && activeCanvas != nullptr)
        {
            const ImVec2 pointer = ImGui::GetIO().MousePos;
            m_uiInputRouter.Update(
                activeScene,
                activeCanvas,
                {viewportSize.x, viewportSize.y},
                {pointer.x - canvasMin.x, pointer.y - canvasMin.y},
                ImGui::IsMouseDown(ImGuiMouseButton_Left),
                ImGui::IsMouseClicked(ImGuiMouseButton_Left),
                ImGui::IsMouseReleased(ImGuiMouseButton_Left),
                hovered);
            for (const UIButtonClickedEvent2D& event : m_uiInputRouter.GetEvents())
            {
                (void)activeScene.DispatchUIButtonClicked(event.entity);
                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                    "UI Button clicked: Entity %llu",
                    static_cast<unsigned long long>(event.entity));
            }
        }
        else
        {
            m_uiInputRouter.Reset();
        }

        if (activeCanvas != nullptr)
        {
            UICanvasLayout2D uiLayout;
            if (ResolveUICanvasLayout(*activeCanvas,
                {viewportSize.x, viewportSize.y}, uiLayout))
            {
                const ImVec2 uiCanvasMin{
                    canvasMin.x + uiLayout.origin.x,
                    canvasMin.y + uiLayout.origin.y
                };
                const ImVec2 uiCanvasMax{
                    uiCanvasMin.x + activeCanvas->referenceResolution.x * uiLayout.scale,
                    uiCanvasMin.y + activeCanvas->referenceResolution.y * uiLayout.scale
                };
                drawList->AddRect(uiCanvasMin, uiCanvasMax,
                    IM_COL32(100, 190, 255, 150), 0.0F, 0, 1.0F);

                activeScene.ForEachEntity([&](const Entity& entity)
                {
                    if (!entity.IsActive()) return;
                    const auto* panel = entity.GetComponent<UIPanel2D>();
                    if (panel == nullptr || !panel->enabled) return;

                    UIRect2D panelRect;
                    if (!ResolveUIPanelRect(*panel, uiLayout, panelRect)) return;

                    const ImVec2 minimum{
                        canvasMin.x + panelRect.position.x,
                        canvasMin.y + panelRect.position.y
                    };
                    const ImVec2 maximum{
                        minimum.x + panelRect.size.x,
                        minimum.y + panelRect.size.y
                    };
                    const ImU32 fill = IM_COL32(
                        static_cast<int>(std::lround(panel->red * 255.0F)),
                        static_cast<int>(std::lround(panel->green * 255.0F)),
                        static_cast<int>(std::lround(panel->blue * 255.0F)),
                        static_cast<int>(std::lround(panel->alpha * 255.0F)));
                    drawList->AddRectFilled(minimum, maximum, fill);
                    drawList->AddRect(minimum, maximum, IM_COL32(225, 230, 245, 210));
                });

                activeScene.ForEachEntity([&](const Entity& entity)
                {
                    if (!entity.IsActive()) return;
                    if (const auto* button = entity.GetComponent<UIButton2D>();
                        button != nullptr && button->enabled && button->IsConfigurationValid())
                    {
                        const ImVec2 minimum{canvasMin.x + uiLayout.origin.x + button->position.x * uiLayout.scale,
                            canvasMin.y + uiLayout.origin.y + button->position.y * uiLayout.scale};
                        const ImVec2 maximum{minimum.x + button->size.x * uiLayout.scale,
                            minimum.y + button->size.y * uiLayout.scale};
                        const UIButtonInteractionState2D& interaction =
                            m_uiInputRouter.GetState(entity.GetId());
                        const float highlight = interaction.pressed ? 0.16F :
                            (interaction.hovered ? 0.08F : 0.0F);
                        const ImU32 fill = IM_COL32(
                            static_cast<int>(std::lround(std::clamp(button->red + highlight, 0.0F, 1.0F) * 255.0F)),
                            static_cast<int>(std::lround(std::clamp(button->green + highlight, 0.0F, 1.0F) * 255.0F)),
                            static_cast<int>(std::lround(std::clamp(button->blue + highlight, 0.0F, 1.0F) * 255.0F)),
                            static_cast<int>(std::lround(button->alpha * 255.0F)));
                        const ImU32 outline = !button->interactable
                            ? IM_COL32(130,135,145,180)
                            : (interaction.pressed ? IM_COL32(255,225,130,255)
                                : (interaction.hovered ? IM_COL32(120,210,255,255)
                                    : IM_COL32(235,240,255,230)));
                        drawList->AddRectFilled(minimum, maximum, fill, 5.0F);
                        drawList->AddRect(minimum, maximum, outline, 5.0F, 0,
                            interaction.hovered || interaction.pressed ? 2.5F : 1.0F);
                        const ImVec2 textSize=ImGui::CalcTextSize(button->text.c_str());
                        const ImVec2 textPos{minimum.x + (maximum.x-minimum.x-textSize.x)/2.0F,
                            minimum.y + (maximum.y-minimum.y-textSize.y)/2.0F};
                        drawList->AddText(textPos, IM_COL32(255,255,255,255), button->text.c_str());
                    }
                    if (const auto* label = entity.GetComponent<UIText2D>();
                        label != nullptr && label->enabled && label->IsConfigurationValid())
                    {
                        const ImVec2 textPos{canvasMin.x + uiLayout.origin.x + label->position.x * uiLayout.scale,
                            canvasMin.y + uiLayout.origin.y + label->position.y * uiLayout.scale};
                        const ImU32 color=IM_COL32(
                            static_cast<int>(std::lround(label->red*255.0F)),
                            static_cast<int>(std::lround(label->green*255.0F)),
                            static_cast<int>(std::lround(label->blue*255.0F)),
                            static_cast<int>(std::lround(label->alpha*255.0F)));
                        drawList->AddText(ImGui::GetFont(), label->fontSize*uiLayout.scale,
                            textPos, color, label->text.c_str());
                    }
                });
            }
        }

        // Draw every configured physics shape without exposing backend handles
        // or Box2D types to Editor code. ColliderSet2D is canonical; legacy
        // single-collider components remain visible for unmigrated authoring data.
        if (m_showPhysicsDebugOverlay)
        {
            activeScene.ForEachEntity([&](const Entity& entity)
            {
                const auto* body = entity.GetComponent<PhysicsBody2D>();
                if (body == nullptr || !body->enabled) return;

                const Transform2D transform = activeScene.GetWorldTransform(entity.GetId()).value_or(entity.GetTransform());
                const float bodyCosine = std::cos(transform.rotation);
                const float bodySine = std::sin(transform.rotation);
                const auto localToWorld = [&](const Vector2& local)
                {
                    return Vector2{
                        transform.position.x + local.x * bodyCosine - local.y * bodySine,
                        transform.position.y + local.x * bodySine + local.y * bodyCosine
                    };
                };
                const auto drawCollider = [&](ColliderShape2D shape, const Vector2& offset,
                    const Vector2& halfExtents, float radius, float localRotation, bool isSensor)
                {
                    const ImU32 color = isSensor
                        ? IM_COL32(80, 220, 255, 255)
                        : IM_COL32(255, 155, 65, 255);
                    if (shape == ColliderShape2D::Box)
                    {
                        const float angle = transform.rotation + localRotation;
                        const float cosine = std::cos(angle);
                        const float sine = std::sin(angle);
                        const Vector2 center = localToWorld(offset);
                        const Vector2 cornersLocal[4]{
                            {-halfExtents.x, -halfExtents.y},
                            { halfExtents.x, -halfExtents.y},
                            { halfExtents.x,  halfExtents.y},
                            {-halfExtents.x,  halfExtents.y}
                        };
                        ImVec2 corners[4]{};
                        for (int corner = 0; corner < 4; ++corner)
                        {
                            const Vector2& point = cornersLocal[corner];
                            corners[corner] = toCanvas({
                                center.x + point.x * cosine - point.y * sine,
                                center.y + point.x * sine + point.y * cosine
                            });
                        }
                        drawList->AddPolyline(corners, 4, color, ImDrawFlags_Closed, 2.0F);
                    }
                    else
                    {
                        const ImVec2 center = toCanvas(localToWorld(offset));
                        drawList->AddCircle(center, radius * zoom, color, 48, 2.0F);
                    }
                };

                if (const auto* colliderSet = entity.GetComponent<ColliderSet2D>())
                {
                    for (const Collider2D& collider : colliderSet->colliders)
                    {
                        drawCollider(collider.shape, collider.offset, collider.halfExtents,
                            collider.radius, collider.rotation, collider.isSensor);
                    }
                    return;
                }

                if (const auto* box = entity.GetComponent<BoxCollider2D>())
                {
                    drawCollider(ColliderShape2D::Box, box->offset, box->halfExtents,
                        0.0F, box->rotation, box->isSensor);
                }
                if (const auto* circle = entity.GetComponent<CircleCollider2D>())
                {
                    drawCollider(ColliderShape2D::Circle, circle->offset, {},
                        circle->radius, 0.0F, circle->isSensor);
                }
            });
        }

        if (!m_isPlaying)
        {
            if (const Entity* selected = activeScene.FindEntity(m_selectedEntity))
            {
                const auto transform = activeScene.GetWorldTransform(selected->GetId()).value_or(selected->GetTransform());
                const auto& render = selected->GetRenderComponent();
                const Vector2 origin{
                    transform.position.x + render.size.x * transform.scale.x * 0.5F,
                    transform.position.y + render.size.y * transform.scale.y * 0.5F
                };
                const ImVec2 pivot = toCanvas(origin);
                if (m_gizmoMode == 1)
                {
                    drawList->AddCircle(pivot, 58.0F, IM_COL32(255, 190, 70, 255), 64, 3.0F);
                    drawList->AddCircleFilled(ImVec2(pivot.x, pivot.y - 58.0F),
                        6.0F, IM_COL32(255, 220, 120, 255));
                    drawList->AddText(ImVec2(pivot.x + 8.0F, pivot.y - 76.0F),
                        IM_COL32(255, 220, 150, 255), "ROT");
                }
                else
                {
                    const float axisLength = 80.0F / zoom;
                    const float axisCosine = m_gizmoLocalSpace ? std::cos(transform.rotation) : 1.0F;
                    const float axisSine = m_gizmoLocalSpace ? std::sin(transform.rotation) : 0.0F;
                    const Vector2 xAxis{axisCosine, axisSine};
                    const Vector2 yAxis{-axisSine, axisCosine};
                    const ImVec2 xEnd = toCanvas(origin + xAxis * axisLength);
                    const ImVec2 yEnd = toCanvas(origin + yAxis * axisLength);
                    const ImU32 xColor = m_gizmoMode == 2
                        ? IM_COL32(255, 175, 65, 255) : IM_COL32(245, 85, 85, 255);
                    const ImU32 yColor = m_gizmoMode == 2
                        ? IM_COL32(75, 190, 255, 255) : IM_COL32(90, 220, 125, 255);
                    drawList->AddLine(pivot, xEnd, xColor, 3.0F);
                    drawList->AddLine(pivot, yEnd, yColor, 3.0F);
                    drawList->AddCircleFilled(xEnd, 6.0F, xColor);
                    drawList->AddCircleFilled(yEnd, 6.0F, yColor);
                    drawList->AddText(ImVec2(xEnd.x + 7.0F, xEnd.y - 7.0F),
                        xColor, "X");
                    drawList->AddText(ImVec2(yEnd.x + 7.0F, yEnd.y - 7.0F),
                        yColor, "Y");
                    drawList->AddCircle(pivot, 4.0F, IM_COL32(255, 230, 130, 255), 0, 2.0F);
                }
            }
        }

        drawList->PopClipRect();
        const char* toolName = m_gizmoMode == 1 ? "Rotate" : (m_gizmoMode == 2 ? "Scale" : "Move");
        ImGui::TextDisabled("%zu entities | Active tool: %s | Drag the handles to edit the selected transform.",
            activeScene.GetEntityCount(), toolName);
    }
    ImGui::End();
}

void EditorApplication::DrawLogWindow()
{
    if (!m_showLogWindow) return;

    ImGui::SetNextWindowSize(ImVec2(620.0F, 300.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Log", &m_showLogWindow))
    {
        const auto entries = m_logger.Snapshot();
        if (ImGui::Button("Clear")) m_logger.Clear();
        ImGui::SameLine();
        ImGui::TextDisabled("%zu / %zu entries", entries.size(), m_logger.Capacity());

        ImGui::SetNextItemWidth(180.0F);
        ImGui::InputTextWithHint("##LogTextFilter", "Filter category or message", m_logTextFilter, sizeof(m_logTextFilter));
        ImGui::SameLine();
        static constexpr const char* levelNames[] = {
            "All levels", "Trace", "Debug", "Info", "Warning", "Error", "Critical"
        };
        ImGui::SetNextItemWidth(130.0F);
        ImGui::Combo("##LogLevelFilter", &m_logLevelFilter, levelNames,
            static_cast<int>(std::size(levelNames)));
        ImGui::Separator();

        const std::string filter = m_logTextFilter;
        std::size_t visibleCount = 0;
        if (ImGui::BeginChild("EngineLogEntries", ImVec2(0.0F, 0.0F), ImGuiChildFlags_Borders))
        {
            for (std::size_t index = 0; index < entries.size(); ++index)
            {
                const LogEntry& entry = entries[index];
                if (m_logLevelFilter != 0 &&
                    static_cast<int>(entry.level) != m_logLevelFilter - 1)
                    continue;
                if (!filter.empty() &&
                    entry.category.find(filter) == std::string::npos &&
                    entry.message.find(filter) == std::string::npos)
                    continue;

                ++visibleCount;
                ImGui::PushID(static_cast<int>(index));
                ImVec4 color(0.82F, 0.85F, 0.90F, 1.0F);
                if (entry.level == LogLevel::Warning) color = ImVec4(1.0F, 0.78F, 0.30F, 1.0F);
                else if (entry.level == LogLevel::Error || entry.level == LogLevel::Critical)
                    color = ImVec4(1.0F, 0.38F, 0.32F, 1.0F);
                else if (entry.level == LogLevel::Debug || entry.level == LogLevel::Trace)
                    color = ImVec4(0.58F, 0.64F, 0.72F, 1.0F);
                ImGui::TextColored(color, "[%llu] [%s] [%s]",
                    static_cast<unsigned long long>(entry.sequence), ToString(entry.level), entry.category.c_str());
                ImGui::SameLine();
                ImGui::TextWrapped("%s", entry.message.c_str());
                ImGui::PopID();
            }
            if (visibleCount == 0)
                ImGui::TextDisabled(entries.empty() ? "No log entries yet." : "No entries match the current filters.");
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void EditorApplication::DrawScriptDiagnostics()
{
    if (!m_showScriptDiagnostics) return;

    ImGui::SetNextWindowSize(ImVec2(420.0F, 220.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Script Diagnostics", &m_showScriptDiagnostics))
    {
        if (!m_isPlaying)
        {
            ImGui::TextDisabled("Start Play Mode to inspect script runtime diagnostics.");
        }
        else
        {
            const auto& diagnostics = m_playScene.GetScriptDiagnostics();
            ImGui::Text("Runtime instances: %zu", m_playScene.GetScriptInstanceCount());
            ImGui::SameLine();
            ImGui::Text("Diagnostics: %zu", diagnostics.size());
            ImGui::Separator();

            if (diagnostics.empty())
            {
                ImGui::TextDisabled("No script diagnostics. All active scripts are healthy.");
            }
            else
            {
                if (ImGui::BeginChild("ScriptDiagnosticList", ImVec2(0.0F, 0.0F), ImGuiChildFlags_Borders))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 150, 110, 255));
                    for (std::size_t index = 0; index < diagnostics.size(); ++index)
                    {
                        ImGui::PushID(static_cast<int>(index));
                        ImGui::BulletText("%s", diagnostics[index].c_str());
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                }
                ImGui::EndChild();
            }
        }
    }
    ImGui::End();
}

Scene& EditorApplication::GetActiveScene()
{
    return m_isPlaying ? m_playScene : m_scene;
}

void EditorApplication::StartPlayMode()
{
    if (m_isPlaying) return;
    StopAudioPreview();
    m_uiInputRouter.Reset();
    m_playScene = m_scene.Clone();
    m_scriptRuntime = std::make_unique<LuaScriptRuntime>(m_assetRegistry);
    m_playScene.SetLogger(&m_runtimeLogger);
    m_playScene.Start(&m_audio, &m_assetRegistry, m_scriptRuntime.get());
    m_isPlaying = true;
    m_logger.Info("Scene", "Play Mode started");
    if (m_playScene.GetScriptDiagnostics().empty())
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Play mode started: C++ components and Lua scripts are running");
    }
    else
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Play mode started with script diagnostic: %.190s",
            m_playScene.GetScriptDiagnostics().front().c_str());
    }
}

void EditorApplication::StopPlayMode()
{
    if (!m_isPlaying) return;
    m_playScene.Stop();
    m_scriptRuntime.reset();
    m_uiInputRouter.Reset();
    m_isPlaying = false;
    m_playScene.Clear();
    m_logger.Info("Scene", "Play Mode stopped");
    if (m_scene.FindEntity(m_selectedEntity) == nullptr) m_selectedEntity = 0;
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Play mode stopped: authoring scene restored unchanged");
}

void EditorApplication::CreateEntity()
{
    if (m_isPlaying) return;
    Entity& entity = m_scene.CreateEntity("Entity");
    m_entitySelection.SelectOnly(entity.GetId());
    m_selectedEntity = entity.GetId();
    MarkSceneDirty();
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Created entity %llu", static_cast<unsigned long long>(m_selectedEntity));
}

void EditorApplication::DuplicateSelectedEntity()
{
    if (m_isPlaying || m_entitySelection.Empty()) return;
    const std::vector<EntityId> sourceIds = m_entitySelection.GetIds();
    std::vector<EntityId> duplicateIds;
    duplicateIds.reserve(sourceIds.size());
    for (EntityId id : sourceIds)
    {
        if (Entity* duplicate = m_scene.DuplicateEntity(id))
            duplicateIds.push_back(duplicate->GetId());
    }
    if (duplicateIds.empty()) return;
    m_entitySelection.SelectAll(duplicateIds);
    m_selectedEntity = m_entitySelection.Primary();
    MarkSceneDirty();
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Duplicated %zu Entity/Entities", duplicateIds.size());
}

void EditorApplication::DeleteSelectedEntity()
{
    if (m_isPlaying || m_entitySelection.Empty()) return;
    const std::vector<EntityId> selectedIds = m_entitySelection.GetIds();
    std::size_t deletedCount = 0;
    for (EntityId id : selectedIds)
        if (m_scene.DestroyEntity(id)) ++deletedCount;
    if (deletedCount == 0) return;
    m_entitySelection.Clear();
    m_selectedEntity = 0;
    MarkSceneDirty();
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Deleted %zu Entity/Entities", deletedCount);
}

void EditorApplication::SelectAllEntities()
{
    if (m_isPlaying) return;
    m_entitySelection.SelectAll(m_scene.GetEntityIds());
    m_selectedEntity = m_entitySelection.Primary();
}

void EditorApplication::PruneEntitySelection()
{
    m_entitySelection.Prune([this](EntityId id) { return m_scene.FindEntity(id) != nullptr; });
    m_selectedEntity = m_entitySelection.Primary();
}

bool EditorApplication::SaveScene()
{
    if (m_isPlaying) return false;
    const auto resolvedPath = ResolveProjectScenePath(
        m_projectFilePath, m_projectConfiguration, m_currentScenePath);
    if (!resolvedPath)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Current scene path must be a safe project-relative path");
        return false;
    }

    std::string error;
    if (SceneSerializer::Save(m_scene, *resolvedPath, &error, &m_componentRegistry))
    {
        m_sceneDirty = false;
        m_logger.Info("Scene", std::string("Saved scene: ") + PathToUtf8(m_currentScenePath));
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Saved scene: %.200s", PathToUtf8(m_currentScenePath).c_str());
        (void)m_assetRegistry.Scan();
        return true;
    }

    m_logger.Error("Scene", std::string("Scene save failed: ") + error);
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Save failed: %.220s", error.c_str());
    return false;
}

void EditorApplication::LoadScene()
{
    if (m_isPlaying) return;
    RequestLoadScene(PathFromUtf8(m_scenePath));
}

bool EditorApplication::LoadSceneFromPath(const std::filesystem::path& path)
{
    const auto resolvedPath = ResolveProjectScenePath(
        m_projectFilePath, m_projectConfiguration, path);
    if (!resolvedPath)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Scene path must be a safe project-relative path");
        return false;
    }

    Scene loadedScene;
    std::string error;
    if (!SceneSerializer::Load(loadedScene, *resolvedPath, &error, &m_componentRegistry))
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Load failed: %.220s", error.c_str());
        return false;
    }

    m_scene = std::move(loadedScene);
    m_scene.SetLogger(&m_logger);
    m_currentScenePath = path;
    std::snprintf(m_scenePath, sizeof(m_scenePath), "%s", PathToUtf8(path).c_str());
    m_selectedEntity = 0;
    m_entitySelection.Clear();
    m_sceneDirty = false;
    ResetSceneHistory();
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Loaded scene: %.200s", PathToUtf8(path).c_str());
    return true;
}

void EditorApplication::RequestNewScene(const std::filesystem::path& path)
{
    if (m_isPlaying) return;
    m_pendingSceneChange = 1;
    m_pendingScenePath = path;
    if (!m_sceneDirty) ApplyPendingSceneChange();
}

void EditorApplication::RequestLoadScene(const std::filesystem::path& path)
{
    if (m_isPlaying) return;
    m_pendingSceneChange = 2;
    m_pendingScenePath = path;
    if (!m_sceneDirty) ApplyPendingSceneChange();
}

void EditorApplication::ApplyPendingSceneChange()
{
    const int change = m_pendingSceneChange;
    const std::filesystem::path path = m_pendingScenePath;
    if (change == 1)
    {
        m_scene = Scene("Untitled Scene");
        m_scene.SetLogger(&m_logger);
        m_currentScenePath = path;
        m_selectedEntity = 0;
        m_entitySelection.Clear();
        m_sceneDirty = true;
        ResetSceneHistory();
        std::snprintf(m_scenePath, sizeof(m_scenePath), "%s", PathToUtf8(path).c_str());
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "New empty scene created (not saved yet)");
    }
    else if (change == 2)
    {
        (void)LoadSceneFromPath(path);
    }
    m_pendingSceneChange = 0;
    m_pendingScenePath.clear();
}

void EditorApplication::DrawUnsavedChangesModal()
{
    if (m_pendingSceneChange == 0) return;
    ImGui::OpenPopup("Unsaved Scene Changes");
    if (ImGui::BeginPopupModal("Unsaved Scene Changes", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("The current scene has unsaved changes.");
        ImGui::TextUnformatted("Save before switching scenes?");
        if (ImGui::Button("Save", ImVec2(110.0F, 0.0F)))
        {
            if (SaveScene())
            {
                ApplyPendingSceneChange();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(110.0F, 0.0F)))
        {
            ApplyPendingSceneChange();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110.0F, 0.0F)))
        {
            m_pendingSceneChange = 0;
            m_pendingScenePath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorApplication::RequestExit()
{
    if (m_scriptSourceDirty || m_sceneDirty)
    {
        m_pendingScriptExit = true;
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Unsaved changes — save or discard before exiting");
        return;
    }
    m_isRunning = false;
}

void EditorApplication::DrawUnsavedScriptExitModal()
{
    if (!m_pendingScriptExit) return;
    ImGui::OpenPopup("Unsaved Changes");
    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("EMBER has unsaved changes.");
        if (m_sceneDirty) ImGui::BulletText("Current Scene");
        if (m_scriptSourceDirty) ImGui::BulletText("Script Source Editor");
        ImGui::TextUnformatted("Save your changes before exiting?");
        if (ImGui::Button("Save and Exit", ImVec2(130.0F, 0.0F)))
        {
            if (m_isPlaying) StopPlayMode();
            if (m_sceneDirty) (void)SaveScene();
            if (m_scriptSourceDirty) SaveScriptSource();
            if (!m_sceneDirty && !m_scriptSourceDirty)
            {
                m_pendingScriptExit = false;
                m_isRunning = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard and Exit", ImVec2(130.0F, 0.0F)))
        {
            m_sceneDirty = false;
            m_scriptSourceDirty = false;
            m_hasScriptValidationResult = false;
            m_scriptValidationMessage.clear();
            m_scriptSourceBuffer.fill('\0');
            m_scriptSourceOriginal.clear();
            m_scriptSourceAssetId.clear();
            m_scriptSourcePath.clear();
            m_pendingScriptExit = false;
            m_isRunning = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110.0F, 0.0F)))
        {
            m_pendingScriptExit = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorApplication::MarkSceneDirty()
{
    if (m_isPlaying) return;
    m_sceneDirty = true;
    std::string snapshot;
    std::string error;
    if (SceneSerializer::SerializeToString(m_scene, snapshot, &error, &m_componentRegistry))
    {
        if (m_sceneUndoHistory.GetEntryCount() == 0)
            m_sceneUndoHistory.Reset(std::move(snapshot));
        else
            (void)m_sceneUndoHistory.Record(std::move(snapshot));
    }
    else
    {
        m_logger.Warning("Undo", "Could not record Scene history snapshot: " + error);
    }
}

void EditorApplication::ResetSceneHistory()
{
    std::string snapshot;
    std::string error;
    if (SceneSerializer::SerializeToString(m_scene, snapshot, &error, &m_componentRegistry))
    {
        m_sceneUndoHistory.Reset(std::move(snapshot));
    }
    else
    {
        m_sceneUndoHistory.Reset({});
        m_logger.Warning("Undo", "Could not initialize Scene history: " + error);
    }
}

void EditorApplication::UndoScene()
{
    if (m_isPlaying) return;
    const std::string* snapshot = m_sceneUndoHistory.Undo();
    if (snapshot == nullptr) return;

    Scene restored;
    std::string error;
    if (!SceneSerializer::LoadFromString(restored, *snapshot, &error, &m_componentRegistry))
    {
        (void)m_sceneUndoHistory.Redo();
        m_logger.Error("Undo", "Could not restore previous Scene snapshot: " + error);
        return;
    }

    restored.SetLogger(&m_logger);
    m_scene = std::move(restored);
    PruneEntitySelection();
    m_sceneDirty = true;
    std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Undo completed");
}

void EditorApplication::RedoScene()
{
    if (m_isPlaying) return;
    const std::string* snapshot = m_sceneUndoHistory.Redo();
    if (snapshot == nullptr) return;

    Scene restored;
    std::string error;
    if (!SceneSerializer::LoadFromString(restored, *snapshot, &error, &m_componentRegistry))
    {
        (void)m_sceneUndoHistory.Undo();
        m_logger.Error("Undo", "Could not restore next Scene snapshot: " + error);
        return;
    }

    restored.SetLogger(&m_logger);
    m_scene = std::move(restored);
    PruneEntitySelection();
    m_sceneDirty = true;
    std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Redo completed");
}

void EditorApplication::LoadStartupScene()
{
    if (m_projectConfiguration.startupScene.empty())
    {
        return;
    }

    const auto startupScene = ProjectConfigurationStore::ResolveStartupScene(
        m_projectFilePath, m_projectConfiguration);
    if (!startupScene)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Startup scene path is not a safe project-relative path");
        return;
    }

    std::error_code filesystemError;
    if (!std::filesystem::exists(*startupScene, filesystemError) || filesystemError)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Startup scene not found: %.190s", startupScene->string().c_str());
        return;
    }

    Scene loadedScene;
    std::string error;
    if (!SceneSerializer::Load(loadedScene, *startupScene, &error, &m_componentRegistry))
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Startup scene load failed: %.180s", error.c_str());
        return;
    }

    m_scene = std::move(loadedScene);
    m_scene.SetLogger(&m_logger);
    m_selectedEntity = 0;
    m_entitySelection.Clear();
    m_sceneDirty = false;
    ResetSceneHistory();
    m_currentScenePath = m_projectConfiguration.startupScene;
    std::snprintf(m_scenePath, sizeof(m_scenePath), "%s",
        PathToUtf8(m_projectConfiguration.startupScene).c_str());
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Startup scene loaded: %.190s", startupScene->string().c_str());
}

void EditorApplication::OpenSelectedScriptSource()
{
    const AssetMetadata* selected = m_assetRegistry.Find(m_selectedAssetId);
    if (selected == nullptr || selected->type != AssetType::Script)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Select a Script asset first");
        return;
    }

    if (m_scriptSourceDirty && m_scriptSourceAssetId != selected->assetId)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Save the current Script before opening another asset");
        return;
    }
    if (m_scriptSourceAssetId == selected->assetId && !m_scriptSourcePath.empty())
    {
        m_showScriptSourceEditor = true;
        return;
    }

    const auto resolved = m_assetRegistry.GetDirectory().Resolve(selected->source);
    if (!resolved)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script source path is invalid or outside the asset directory");
        return;
    }

    std::error_code filesystemError;
    const auto sourceSize = std::filesystem::file_size(*resolved, filesystemError);
    if (filesystemError || sourceSize > m_scriptSourceBuffer.size() - 1)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script source is unavailable or exceeds the 64 KiB Editor limit");
        return;
    }

    std::ifstream source(*resolved, std::ios::binary);
    if (!source)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Could not open Script source");
        return;
    }
    // Read into a temporary buffer so a failed open/partial read cannot corrupt
    // the currently displayed Script source or its dirty-state snapshot.
    std::array<char, 65536> openedBuffer{};
    source.read(openedBuffer.data(), static_cast<std::streamsize>(sourceSize));
    if (source.gcount() != static_cast<std::streamsize>(sourceSize) || source.bad())
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Could not read complete Script source");
        return;
    }
    const auto sourceEnd = openedBuffer.begin() +
        static_cast<std::ptrdiff_t>(sourceSize);
    if (std::find(openedBuffer.begin(), sourceEnd, '\0') != sourceEnd)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script source contains a NUL byte and cannot be opened as text");
        return;
    }
    openedBuffer[static_cast<std::size_t>(sourceSize)] = '\0';
    m_scriptSourceBuffer = openedBuffer;
    m_scriptSourceOriginal.assign(openedBuffer.data(), static_cast<std::size_t>(sourceSize));
    m_scriptSourceAssetId = selected->assetId;
    m_scriptSourcePath = selected->source;
    m_scriptSourceDirty = false;
    m_hasScriptValidationResult = false;
    m_scriptValidationMessage.clear();
    m_showScriptSourceEditor = true;
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Opened Script source: %.180s", selected->assetId.c_str());
}

void EditorApplication::ReloadScriptSourceFromDisk()
{
    if (m_scriptSourceAssetId.empty() || m_scriptSourcePath.empty())
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "No Script source is open");
        return;
    }
    if (m_scriptSourceDirty)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Discard unsaved edits before reloading from disk");
        return;
    }
    const AssetMetadata* metadata = m_assetRegistry.Find(m_scriptSourceAssetId);
    if (metadata == nullptr || metadata->type != AssetType::Script ||
        metadata->source != m_scriptSourcePath)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script asset changed or is no longer indexed; refresh before reloading");
        return;
    }
    const auto resolved = m_assetRegistry.GetDirectory().Resolve(m_scriptSourcePath);
    if (!resolved)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script source path is invalid or outside the asset directory");
        return;
    }
    std::error_code filesystemError;
    const auto sourceSize = std::filesystem::file_size(*resolved, filesystemError);
    if (filesystemError || sourceSize > m_scriptSourceBuffer.size() - 1)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script source is unavailable or exceeds the 64 KiB Editor limit");
        return;
    }
    std::ifstream source(*resolved, std::ios::binary);
    if (!source)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Could not open Script source");
        return;
    }
    std::array<char, 65536> reloadedBuffer{};
    source.read(reloadedBuffer.data(), static_cast<std::streamsize>(sourceSize));
    if (source.gcount() != static_cast<std::streamsize>(sourceSize) || source.bad())
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Could not read complete Script source");
        return;
    }
    reloadedBuffer[static_cast<std::size_t>(sourceSize)] = '\0';
    m_scriptSourceBuffer = reloadedBuffer;
    m_scriptSourceOriginal.assign(reloadedBuffer.data(), static_cast<std::size_t>(sourceSize));
    m_hasScriptValidationResult = false;
    m_scriptValidationMessage.clear();
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Reloaded Script source from disk: %.160s", m_scriptSourceAssetId.c_str());
}

void EditorApplication::ValidateScriptSource()
{
    if (m_scriptSourceAssetId.empty() || m_scriptSourcePath.empty())
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "No Script source is open");
        return;
    }
    const AssetMetadata* metadata = m_assetRegistry.Find(m_scriptSourceAssetId);
    if (metadata == nullptr || metadata->type != AssetType::Script ||
        metadata->source != m_scriptSourcePath)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script asset changed or is no longer indexed; refresh before validating");
        return;
    }
    if (metadata->scriptLanguage != "lua")
    {
        m_hasScriptValidationResult = false;
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "No validator is available for Script language: %.150s",
            metadata->scriptLanguage.c_str());
        return;
    }

    const std::size_t sourceLength =
        std::char_traits<char>::length(m_scriptSourceBuffer.data());
    m_scriptValidationSucceeded = LuaScriptRuntime::ValidateSource(
        std::string_view(m_scriptSourceBuffer.data(), sourceLength),
        m_scriptSourcePath.generic_string(), &m_scriptValidationMessage,
        &m_scriptValidationLine, &m_scriptValidationColumn);
    m_hasScriptValidationResult = true;
    if (m_scriptValidationSucceeded)
    {
        m_scriptValidationLine = 0;
        m_scriptValidationColumn = 0;
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script syntax is valid: %.170s", m_scriptSourceAssetId.c_str());
    }
    else
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script syntax error at line %zu, column %zu",
            m_scriptValidationLine, m_scriptValidationColumn);
    }
}

void EditorApplication::SaveScriptSource()
{
    if (m_scriptSourceAssetId.empty() || m_scriptSourcePath.empty())
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "No Script source is open");
        return;
    }
    if (m_isPlaying)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Stop Play Mode before saving Script source");
        return;
    }

    const AssetMetadata* metadata = m_assetRegistry.Find(m_scriptSourceAssetId);
    if (metadata == nullptr || metadata->type != AssetType::Script ||
        metadata->source != m_scriptSourcePath)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script asset changed or is no longer indexed; refresh before saving");
        return;
    }

    const auto resolved = m_assetRegistry.GetDirectory().Resolve(m_scriptSourcePath);
    if (!resolved)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script source path is invalid or outside the asset directory");
        return;
    }

    // Refuse to overwrite a file that changed outside the Editor since it was opened/saved.
    std::ifstream currentSource(*resolved, std::ios::binary);
    if (!currentSource)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Could not verify Script source before saving");
        return;
    }
    std::error_code diskSizeError;
    const auto diskSize = std::filesystem::file_size(*resolved, diskSizeError);
    if (diskSizeError || diskSize > m_scriptSourceBuffer.size() - 1)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script source changed or exceeds the 64 KiB Editor limit; reload before saving");
        return;
    }
    std::array<char, 65536> diskBuffer{};
    currentSource.read(diskBuffer.data(), static_cast<std::streamsize>(diskSize));
    const bool readComplete =
        currentSource.gcount() == static_cast<std::streamsize>(diskSize) && !currentSource.bad();
    const int trailingByte = readComplete ? currentSource.peek() : std::char_traits<char>::eof();
    const std::string diskContents(diskBuffer.data(),
        static_cast<std::size_t>(std::max<std::streamsize>(0, currentSource.gcount())));
    if (!readComplete || trailingByte != std::char_traits<char>::eof() ||
        diskContents != m_scriptSourceOriginal)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Script source changed on disk; discard edits and reload before saving");
        return;
    }

    const std::size_t sourceLength = std::char_traits<char>::length(m_scriptSourceBuffer.data());
    const std::string_view editedSource(m_scriptSourceBuffer.data(), sourceLength);
    if (!ember::WriteFileAtomically(*resolved, editedSource))
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Could not atomically replace Script source; original file was preserved");
        return;
    }

    m_scriptSourceOriginal.assign(editedSource);
    m_scriptSourceDirty = false;
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Saved Script source: %.180s", m_scriptSourceAssetId.c_str());
}

void EditorApplication::DrawScriptSourceEditor()
{
    if (!m_showScriptSourceEditor) return;
    bool keepOpen = true;
    if (ImGui::Begin("Script Source Editor", &keepOpen))
    {
        if (m_scriptSourceAssetId.empty())
        {
            ImGui::TextDisabled("Choose a Script asset in Asset Browser and open it here.");
        }
        else
        {
            ImGui::Text("%s%s", m_scriptSourceAssetId.c_str(),
                m_scriptSourceDirty ? " *" : "");
            ImGui::SameLine();
            ImGui::TextDisabled("%s", m_scriptSourcePath.generic_string().c_str());
            ImGui::BeginDisabled(!m_scriptSourceDirty || m_isPlaying);
            if (ImGui::Button("Save"))
                SaveScriptSource();
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(m_isPlaying);
            if (ImGui::Button("Validate"))
                ValidateScriptSource();
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(m_scriptSourceDirty);
            if (ImGui::Button("Reload from Disk"))
                ReloadScriptSourceFromDisk();
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("Text source only | 64 KiB max");
            if (ImGui::InputTextMultiline("##ScriptSource",
                m_scriptSourceBuffer.data(), m_scriptSourceBuffer.size(),
                ImVec2(-1.0F, 420.0F), ImGuiInputTextFlags_AllowTabInput))
            {
                m_scriptSourceDirty = true;
                m_hasScriptValidationResult = false;
                m_scriptValidationMessage.clear();
            }
            if (m_hasScriptValidationResult)
            {
                if (m_scriptValidationSucceeded)
                {
                    ImGui::TextColored(ImVec4(0.35F, 0.85F, 0.45F, 1.0F),
                        "Syntax valid — Lua source parsed successfully.");
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0F, 0.35F, 0.30F, 1.0F),
                        "Syntax error at line %zu, column %zu",
                        m_scriptValidationLine, m_scriptValidationColumn);
                    ImGui::TextWrapped("%s", m_scriptValidationMessage.c_str());
                }
            }
            if (m_scriptSourceDirty)
            {
                ImGui::TextColored(ImVec4(1.0F, 0.75F, 0.25F, 1.0F),
                    "Unsaved changes — save before opening another Script asset.");
                if (ImGui::Button("Discard Changes"))
                {
                    m_scriptSourceDirty = false;
                    m_hasScriptValidationResult = false;
                    m_scriptValidationMessage.clear();
                    m_scriptSourceBuffer.fill('\0');
                    m_scriptSourceOriginal.clear();
                    m_scriptSourceAssetId.clear();
                    m_scriptSourcePath.clear();
                    m_allowScriptSourceClose = true;
                    keepOpen = false;
                }
            }
            if (m_isPlaying)
                ImGui::TextDisabled("Stop Play Mode to edit or save source files.");
        }
    }
    ImGui::End();
    if (!keepOpen)
    {
        if (m_scriptSourceDirty && !m_allowScriptSourceClose)
        {
            m_showScriptSourceEditor = true;
            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                "Unsaved Script changes kept open; save or discard them explicitly");
        }
        else
        {
            m_showScriptSourceEditor = false;
        }
        m_allowScriptSourceClose = false;
    }
}

void EditorApplication::RefreshAssets()
{
    StopAudioPreview();
    ClearPreviewTextures();
    const bool scanSucceeded = m_assetRegistry.Scan();
    const bool importScanSucceeded = m_assetImportService.Scan();
    if (!m_selectedAssetId.empty() && m_assetRegistry.Find(m_selectedAssetId) == nullptr)
    {
        m_selectedAssetId.clear();
        m_importSettingsAssetId.clear();
    }

    if (scanSucceeded && importScanSucceeded)
    {
        m_logger.Info("Assets", "Asset scan completed");
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Asset scan complete: %zu assets, %zu diagnostics",
            m_assetRegistry.GetAssetCount(), m_assetRegistry.GetDiagnostics().size());
    }
    else
    {
        const auto& diagnostics = m_assetRegistry.GetDiagnostics();
        const char* reason = diagnostics.empty() ? "Asset scan failed." : diagnostics.front().message.c_str();
        m_logger.Error("Assets", reason);
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Asset scan failed: %.210s", reason);
    }
}


void EditorApplication::LoadInputSettings()
{
    m_projectLoadFailed = false;
    std::string error;
    auto loaded = ProjectConfigurationStore::Load(m_projectFilePath, &error);
    if (!loaded)
    {
        // A missing project file is a first-run case. Other invalid files are
        // preserved so the editor never silently overwrites user data.
        if (std::filesystem::exists(m_projectFilePath))
        {
            m_projectLoadFailed = true;
            std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                "Project settings could not be loaded; saving is disabled: %.160s",
                error.c_str());
            return;
        }

        m_projectConfiguration = ProjectConfiguration{};
        std::snprintf(m_projectName, sizeof(m_projectName), "%s", m_projectConfiguration.name.c_str());
        std::snprintf(m_assetDirectory, sizeof(m_assetDirectory), "%s", PathToUtf8(m_projectConfiguration.assetDirectory).c_str());
        m_startupScenePath[0] = '\0';
        m_inputMap = InputMap::CreateDefaultMovementMap();
        (void)SaveInputSettings();
        return;
    }

    m_projectConfiguration = std::move(*loaded);
    std::snprintf(m_projectName, sizeof(m_projectName), "%s", m_projectConfiguration.name.c_str());
    std::snprintf(m_assetDirectory, sizeof(m_assetDirectory), "%s", PathToUtf8(m_projectConfiguration.assetDirectory).c_str());
    std::snprintf(m_startupScenePath, sizeof(m_startupScenePath), "%s",
        PathToUtf8(m_projectConfiguration.startupScene).c_str());

    const auto resolvedAssetDirectory = ProjectConfigurationStore::ResolveAssetDirectory(
        m_projectFilePath, m_projectConfiguration);
    if (resolvedAssetDirectory)
    {
        m_assetRegistry = AssetRegistry(*resolvedAssetDirectory);
        m_assetImportService = AssetImportService(*resolvedAssetDirectory);
    }
    else
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Asset directory is invalid; using default asset root");
    }
    if (!m_projectConfiguration.inputBindingsConfigured)
    {
        m_inputMap = InputMap::CreateDefaultMovementMap();
    }
    else
    {
        InputMap loadedMap;
        for (const auto& [action, keys] : m_projectConfiguration.inputBindings)
        {
            for (const std::int32_t key : keys)
            {
                if (!loadedMap.BindAction(action, static_cast<SDL_Scancode>(key)))
                {
                    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                        "Invalid input binding ignored: %.180s", action.c_str());
                }
            }
        }
        m_inputMap = std::move(loadedMap);
    }

    m_customInputActions.clear();
    for (const auto& [action, keys] : m_projectConfiguration.inputBindings)
    {
        (void)keys;
        if (action != "MoveLeft" && action != "MoveRight" &&
            action != "MoveUp" && action != "MoveDown")
        {
            m_customInputActions.push_back(action);
            (void)m_inputMap.CreateAction(action);
        }
    }
}

bool EditorApplication::SaveInputSettings()
{
    if (m_projectLoadFailed)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Project save blocked because the existing project file failed to load");
        return false;
    }
    m_projectConfiguration.inputBindings.clear();
    m_projectConfiguration.inputBindingsConfigured = true;
    for (const auto& [action, keys] : m_inputMap.GetAllBindings())
    {
        auto& storedKeys = m_projectConfiguration.inputBindings[action];
        for (const SDL_Scancode key : keys)
        {
            storedKeys.push_back(static_cast<std::int32_t>(key));
        }
    }
    for (const std::string& action : m_customInputActions)
    {
        m_projectConfiguration.inputBindings.try_emplace(action);
    }

    std::string error;
    if (!ProjectConfigurationStore::Save(
            m_projectFilePath, m_projectConfiguration, &error))
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Input settings save failed: %.190s", error.c_str());
        return false;
    }
    return true;
}

bool EditorApplication::SaveProjectSettings()
{
    if (m_projectLoadFailed)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Project save blocked; restore or repair the project file first");
        return false;
    }
    const std::string name = m_projectName;
    const std::filesystem::path assetDirectory = PathFromUtf8(m_assetDirectory);
    const std::filesystem::path startupScene = PathFromUtf8(m_startupScenePath);
    if (name.empty())
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Project name must not be empty");
        return false;
    }

    ProjectConfiguration candidate = m_projectConfiguration;
    candidate.name = name;
    candidate.assetDirectory = assetDirectory;
    candidate.startupScene = startupScene;

    // Validate every project path before SaveInputSettings writes the file.
    const auto resolvedAssetDirectory = ProjectConfigurationStore::ResolveAssetDirectory(
        m_projectFilePath, candidate);
    if (!resolvedAssetDirectory)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Asset directory must be a safe relative path");
        return false;
    }
    if (!startupScene.empty() &&
        !ProjectConfigurationStore::ResolveStartupScene(m_projectFilePath, candidate))
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Startup scene must be a safe relative path");
        return false;
    }

    const ProjectConfiguration previous = m_projectConfiguration;
    m_projectConfiguration = std::move(candidate);
    if (!SaveInputSettings())
    {
        m_projectConfiguration = previous;
        return false;
    }

    m_assetRegistry = AssetRegistry(*resolvedAssetDirectory);
    m_assetImportService = AssetImportService(*resolvedAssetDirectory);
    if (!startupScene.empty())
    {
        const auto resolvedStartupScene = ProjectConfigurationStore::ResolveStartupScene(
            m_projectFilePath, m_projectConfiguration);
        if (resolvedStartupScene)
        {
            std::snprintf(m_scenePath, sizeof(m_scenePath), "%s",
                PathToUtf8(m_projectConfiguration.startupScene).c_str());
        }
    }
    RefreshAssets();
    return true;
}

void EditorApplication::LoadSelectedTextureSettings()
{
    m_importSettingsAssetId = m_selectedAssetId;
    const AssetMetadata* metadata = m_assetImportService.FindAsset(m_selectedAssetId);
    if (metadata == nullptr || metadata->type != AssetType::Texture)
    {
        m_textureFilter = 0;
        m_premultiplyAlpha = false;
        return;
    }

    m_textureFilter = metadata->texture.filter == TextureFilter::Linear ? 1 : 0;
    m_premultiplyAlpha = metadata->texture.premultiplyAlpha;
}
} // namespace ember
