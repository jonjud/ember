#include "Editor/EditorApplication.hpp"

#include "Engine/Scene/SceneSerializer.hpp"
#include "Engine/Platform/Input.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cmath>
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

float SnapValue(float value, float increment)
{
    return increment > 0.0F ? std::round(value / increment) * increment : value;
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
                    if (m_replaceActionOnRebind)
                    {
                        m_inputMap.ClearAction(m_rebindAction);
                    }
                    const std::string action = m_rebindAction;
                    const bool added = m_inputMap.BindAction(action, event.key.scancode);
                    const char* keyName = SDL_GetScancodeName(event.key.scancode);
                    if (added)
                    {
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "Bound %.100s to %.100s", action.c_str(),
                            keyName != nullptr ? keyName : "Unknown");
                    }
                    else
                    {
                        std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                            "%.180s already uses that key", action.c_str());
                    }
                    m_rebindAction.clear();
                }
                continue;
            }

            const ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureKeyboard && !io.WantTextInput)
            {
                if (event.key.key == SDLK_F5 && (event.key.mod & SDL_KMOD_SHIFT) != 0)
                {
                    StopPlayMode();
                }
                else if (event.key.key == SDLK_F5)
                {
                    StartPlayMode();
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
    DrawSceneHierarchy();
    DrawInspector();
    DrawAssetBrowser();
    DrawInputSettings();
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
        ImGui::BeginDisabled(m_isPlaying);
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) SaveScene();
        if (ImGui::MenuItem("Load Scene")) LoadScene();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) m_isRunning = false;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Entity"))
    {
        if (ImGui::MenuItem("Create Empty Entity")) CreateEntity();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene"))
    {
        if (!m_isPlaying && ImGui::MenuItem("Play", "F5")) StartPlayMode();
        if (m_isPlaying && ImGui::MenuItem("Stop", "Shift+F5")) StopPlayMode();
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
        Scene& activeScene = GetActiveScene();
        ImGui::Text("%s", activeScene.GetName().c_str());
        ImGui::SameLine();
        ImGui::BeginDisabled(m_isPlaying);
        if (ImGui::SmallButton("+")) CreateEntity();
        ImGui::EndDisabled();
        ImGui::Separator();

        activeScene.ForEachEntity([this](Entity& entity)
        {
            const bool selected = entity.GetId() == m_selectedEntity;
            char label[160]{};
            std::snprintf(label, sizeof(label), "%s##%llu",
                entity.GetName().c_str(),
                static_cast<unsigned long long>(entity.GetId()));
            if (ImGui::Selectable(label, selected)) m_selectedEntity = entity.GetId();
        });

        if (activeScene.IsEmpty()) ImGui::TextDisabled("No entities. Press + to create one.");
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

            ImGui::SeparatorText("Movement");
            auto& movement = entity->GetMovementComponent();
            ImGui::Checkbox("Enable keyboard movement", &movement.enabled);
            ImGui::DragFloat("Movement speed", &movement.speed, 1.0F, 0.0F, 2000.0F);

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
        ImGui::EndDisabled();
    }
    ImGui::End();
}

void EditorApplication::DrawInputSettings()
{
    ImGui::SetNextWindowPos(ImVec2(1080.0F, 620.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0F, 260.0F), ImGuiCond_FirstUseEver);
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
            }
            ImGui::PopID();
        };

        if (ImGui::BeginTable("InputActionBindings", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 92.0F);
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

        ImGui::Separator();
        ImGui::SetNextItemWidth(180.0F);
        ImGui::InputText("New action", m_newInputActionName, sizeof(m_newInputActionName));
        ImGui::SameLine();
        if (ImGui::Button("Add + Bind"))
        {
            const std::string action = m_newInputActionName;
            if (!action.empty())
            {
                if (std::find(m_customInputActions.begin(), m_customInputActions.end(), action)
                    == m_customInputActions.end())
                {
                    m_customInputActions.push_back(action);
                }
                m_rebindAction = action;
                m_replaceActionOnRebind = false;
                std::snprintf(m_statusMessage, sizeof(m_statusMessage),
                    "Press a key to bind action: %.180s", action.c_str());
            }
        }
        ImGui::TextDisabled("Rebind replaces keys; +Key adds another. Changes are session-only for now.");
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
        ImGui::BeginDisabled(m_isPlaying);
        ImGui::Text("Scene file: %s", m_scenePath);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220.0F);
        ImGui::InputText("Path", m_scenePath, sizeof(m_scenePath));
        ImGui::SameLine();
        if (ImGui::SmallButton("Save Scene")) SaveScene();
        ImGui::SameLine();
        if (ImGui::SmallButton("Load Scene")) LoadScene();
        ImGui::EndDisabled();
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

    SDL_Surface* surface = IMG_Load(resolvedPath->string().c_str());
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
                "PLAY MODE - isolated scene preview (runtime simulation is not connected yet)");
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
                    const auto& transform = selected->GetTransform();
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
                        const Vector2 axes[2]{{1.0F, 0.0F}, {0.0F, 1.0F}};
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
                    const auto& transform = entity->GetTransform();
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

                    const auto& transform = entity.GetTransform();
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
                m_selectedEntity = hitEntity;
                if (!m_isPlaying && hitEntity != 0)
                {
                    if (Entity* entity = activeScene.FindEntity(hitEntity))
                    {
                        m_dragEntityId = hitEntity;
                        m_dragStartMouseWorld = worldMouse;
                        m_dragStartEntityPosition = entity->GetTransform().position;
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
                auto& transform = entity->GetTransform();
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

            const auto& transform = entity.GetTransform();
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
                drawList->AddImageQuad(
                    textureId,
                    corners[0], corners[1], corners[2], corners[3],
                    ImVec2(0.0F, 0.0F), ImVec2(1.0F, 0.0F),
                    ImVec2(1.0F, 1.0F), ImVec2(0.0F, 1.0F),
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

        if (!m_isPlaying)
        {
            if (const Entity* selected = activeScene.FindEntity(m_selectedEntity))
            {
                const auto& transform = selected->GetTransform();
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

Scene& EditorApplication::GetActiveScene()
{
    return m_isPlaying ? m_playScene : m_scene;
}

void EditorApplication::StartPlayMode()
{
    if (m_isPlaying) return;
    m_playScene = m_scene.Clone();
    m_isPlaying = true;
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Play mode: WASD / arrow keys move the selected entity in an isolated scene copy");
}

void EditorApplication::StopPlayMode()
{
    if (!m_isPlaying) return;
    m_isPlaying = false;
    m_playScene.Clear();
    if (m_scene.FindEntity(m_selectedEntity) == nullptr) m_selectedEntity = 0;
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Play mode stopped: authoring scene restored unchanged");
}

void EditorApplication::CreateEntity()
{
    if (m_isPlaying) return;
    Entity& entity = m_scene.CreateEntity("Entity");
    m_selectedEntity = entity.GetId();
    std::snprintf(m_statusMessage, sizeof(m_statusMessage),
        "Created entity %llu", static_cast<unsigned long long>(m_selectedEntity));
}

void EditorApplication::SaveScene()
{
    if (m_isPlaying) return;
    std::string error;
    if (SceneSerializer::Save(m_scene, m_scenePath, &error))
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Saved scene: %.200s", m_scenePath);
    else
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Save failed: %.220s", error.c_str());
}

void EditorApplication::LoadScene()
{
    if (m_isPlaying) return;
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
