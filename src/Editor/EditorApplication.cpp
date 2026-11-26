#include "Editor/EditorApplication.hpp"

#include "Engine/Scene/SceneSerializer.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <vector>

namespace
{
constexpr int kWindowWidth = 1440;
constexpr int kWindowHeight = 900;

const char* AssetTypeName(ember::AssetType type)
{
    switch (type)
    {
    case ember::AssetType::Texture: return "Texture";
    case ember::AssetType::Audio: return "Audio";
    case ember::AssetType::Scene: return "Scene";
    }
    return "Unknown";
}

bool MatchesAssetType(ember::AssetType type, int filter)
{
    return filter == 0 ||
        (filter == 1 && type == ember::AssetType::Texture) ||
        (filter == 2 && type == ember::AssetType::Audio) ||
        (filter == 3 && type == ember::AssetType::Scene);
}
}

namespace ember
{
EditorApplication::~EditorApplication()
{
    Shutdown();
}

bool EditorApplication::Initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "EMBER Editor: SDL initialization failed: " << SDL_GetError() << '\n';
        return false;
    }

    m_window = SDL_CreateWindow("EMBER Editor", kWindowWidth, kWindowHeight, SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr)
    {
        std::cerr << "EMBER Editor: Window creation failed: " << SDL_GetError() << '\n';
        Shutdown();
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (m_renderer == nullptr)
    {
        std::cerr << "EMBER Editor: Renderer creation failed: " << SDL_GetError() << '\n';
        Shutdown();
        return false;
    }

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

    RefreshAssets();
    m_isRunning = true;
    return true;
}

void EditorApplication::Run()
{
    while (m_isRunning)
    {
        ProcessEvents();
        Render();
    }
}

void EditorApplication::Shutdown()
{
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

void EditorApplication::ProcessEvents()
{
    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT)
        {
            m_isRunning = false;
        }
    }
}

void EditorApplication::Render()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    DrawMenuBar();
    DrawSceneHierarchy();
    DrawInspector();
    DrawAssetBrowser();
    DrawViewport();

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

void EditorApplication::DrawMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) SaveScene();
        if (ImGui::MenuItem("Load Scene")) LoadScene();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) m_isRunning = false;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Entity"))
    {
        if (ImGui::MenuItem("Create Empty Entity")) CreateEntity();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Assets"))
    {
        if (ImGui::MenuItem("Refresh Asset Browser")) RefreshAssets();
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorApplication::DrawSceneHierarchy()
{
    ImGui::SetNextWindowPos(ImVec2(0.0F, 24.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280.0F, 590.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Hierarchy"))
    {
        ImGui::Text("%s", m_scene.GetName().c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("+")) CreateEntity();
        ImGui::Separator();

        m_scene.ForEachEntity([this](Entity& entity)
        {
            const bool selected = entity.GetId() == m_selectedEntity;
            char label[160]{};
            std::snprintf(label, sizeof(label), "%s##%llu",
                entity.GetName().c_str(),
                static_cast<unsigned long long>(entity.GetId()));
            if (ImGui::Selectable(label, selected)) m_selectedEntity = entity.GetId();
        });

        if (m_scene.IsEmpty()) ImGui::TextDisabled("No entities. Press + to create one.");
    }
    ImGui::End();
}

void EditorApplication::DrawInspector()
{
    ImGui::SetNextWindowPos(ImVec2(1080.0F, 24.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0F, 590.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Inspector"))
    {
        Entity* entity = m_scene.FindEntity(m_selectedEntity);
        if (entity == nullptr)
        {
            ImGui::TextDisabled("Select an entity to inspect its properties.");
        }
        else
        {
            char name[128]{};
            std::snprintf(name, sizeof(name), "%s", entity->GetName().c_str());
            if (ImGui::InputText("Name", name, sizeof(name))) entity->SetName(name);

            bool active = entity->IsActive();
            if (ImGui::Checkbox("Active", &active)) entity->SetActive(active);

            ImGui::SeparatorText("Transform");
            auto& transform = entity->GetTransform();
            ImGui::DragFloat2("Position", &transform.position.x, 0.1F);
            ImGui::DragFloat2("Scale", &transform.scale.x, 0.01F);
            ImGui::DragFloat("Rotation (rad)", &transform.rotation, 0.01F);

            ImGui::SeparatorText("Renderer");
            auto& render = entity->GetRenderComponent();
            ImGui::Checkbox("Visible", &render.visible);
            ImGui::DragFloat2("Size", &render.size.x, 0.5F, 0.0F, 4096.0F);
            int color[4]{render.red, render.green, render.blue, render.alpha};
            if (ImGui::InputInt4("Tint RGBA", color))
            {
                render.red = static_cast<std::uint8_t>(std::clamp(color[0], 0, 255));
                render.green = static_cast<std::uint8_t>(std::clamp(color[1], 0, 255));
                render.blue = static_cast<std::uint8_t>(std::clamp(color[2], 0, 255));
                render.alpha = static_cast<std::uint8_t>(std::clamp(color[3], 0, 255));
            }

            ImGui::Separator();
            if (ImGui::Button("Delete Entity"))
            {
                const EntityId id = entity->GetId();
                (void)m_scene.DestroyEntity(id);
                m_selectedEntity = 0;
            }
        }
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
        ImGui::Combo("Type", &m_assetTypeFilter, "All\0Textures\0Audio\0Scenes\0");

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
                    m_selectedAssetId = metadata->assetId;
                    LoadSelectedTextureSettings();
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(AssetTypeName(metadata->type));
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

            if (selected->type == AssetType::Texture)
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
            else
            {
                ImGui::TextDisabled("Import settings are currently available for Texture assets only.");
            }
        }
        else if (!m_selectedAssetId.empty())
        {
            ImGui::TextDisabled("The previously selected asset is no longer indexed.");
        }

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
        ImGui::Text("Scene file: %s", m_scenePath);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220.0F);
        ImGui::InputText("Path", m_scenePath, sizeof(m_scenePath));
        ImGui::SameLine();
        if (ImGui::SmallButton("Save Scene")) SaveScene();
        ImGui::SameLine();
        if (ImGui::SmallButton("Load Scene")) LoadScene();
    }
    ImGui::End();
}

void EditorApplication::DrawViewport()
{
    ImGui::SetNextWindowPos(ImVec2(280.0F, 24.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800.0F, 620.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("2D Viewport"))
    {
        ImGui::TextDisabled("Viewport rendering and camera controls are coming next.");
        ImGui::Text("Entities in scene: %zu", m_scene.GetEntityCount());
    }
    ImGui::End();
}

void EditorApplication::CreateEntity()
{
    Entity& entity = m_scene.CreateEntity("Entity");
    m_selectedEntity = entity.GetId();
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Created entity %llu", static_cast<unsigned long long>(m_selectedEntity));
}

void EditorApplication::SaveScene()
{
    std::string error;
    if (SceneSerializer::Save(m_scene, m_scenePath, &error))
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Saved scene: %.200s", m_scenePath);
    else
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Save failed: %.220s", error.c_str());
}

void EditorApplication::LoadScene()
{
    Scene loadedScene;
    std::string error;
    if (SceneSerializer::Load(loadedScene, m_scenePath, &error))
    {
        m_scene = std::move(loadedScene);
        m_selectedEntity = 0;
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Loaded scene: %.200s", m_scenePath);
    }
    else
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Load failed: %.220s", error.c_str());
    }
}

void EditorApplication::RefreshAssets()
{
    const bool scanSucceeded = m_assetRegistry.Scan();
    const bool importScanSucceeded = m_assetImportService.Scan();
    if (!m_selectedAssetId.empty() && m_assetRegistry.Find(m_selectedAssetId) == nullptr)
    {
        m_selectedAssetId.clear();
        m_importSettingsAssetId.clear();
    }

    if (scanSucceeded && importScanSucceeded)
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
            "Asset scan complete: %zu assets, %zu diagnostics",
            m_assetRegistry.GetAssetCount(), m_assetRegistry.GetDiagnostics().size());
    }
    else
    {
        const auto& diagnostics = m_assetRegistry.GetDiagnostics();
        const char* reason = diagnostics.empty() ? "Asset scan failed." : diagnostics.front().message.c_str();
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Asset scan failed: %.210s", reason);
    }
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
