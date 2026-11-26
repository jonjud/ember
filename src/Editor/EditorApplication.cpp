#include "Editor/EditorApplication.hpp"

#include "Engine/Scene/SceneSerializer.hpp"

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
        ImGui::TextDisabled("Middle-drag: pan | Mouse wheel: zoom | Left-click: select | Left-drag selected entity: move");
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
                {
                    hitEntity = entity.GetId();
                }
            });
            m_selectedEntity = hitEntity;
            m_dragEntityId = 0;

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

        if (!m_isPlaying && m_dragEntityId != 0 &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            if (Entity* entity = activeScene.FindEntity(m_dragEntityId))
            {
                const Vector2 movement = worldMouse - m_dragStartMouseWorld;
                entity->GetTransform().position = m_dragStartEntityPosition + movement;
            }
            else
            {
                m_dragEntityId = 0;
            }
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_dragEntityId = 0;
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

        drawList->PopClipRect();
        ImGui::TextDisabled("%zu entities | Active, visible render components are shown. Texture previews use project import settings.",
            activeScene.GetEntityCount());
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
        "Play mode started: running an isolated copy of the current scene");
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
