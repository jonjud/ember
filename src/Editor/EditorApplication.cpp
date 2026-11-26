#include "Editor/EditorApplication.hpp"

#include "Engine/Scene/SceneSerializer.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cstdio>
#include <iostream>

namespace
{
constexpr int kWindowWidth = 1440;
constexpr int kWindowHeight = 900;
constexpr float kMenuHeight = 24.0F;
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
        std::cerr << "EMBER Editor: SDL initialization failed: " << SDL_GetError() << '\\n';
        return false;
    }

    m_window = SDL_CreateWindow("EMBER Editor", kWindowWidth, kWindowHeight, SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr)
    {
        std::cerr << "EMBER Editor: Window creation failed: " << SDL_GetError() << '\\n';
        Shutdown();
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (m_renderer == nullptr)
    {
        std::cerr << "EMBER Editor: Renderer creation failed: " << SDL_GetError() << '\\n';
        Shutdown();
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer))
    {
        std::cerr << "EMBER Editor: ImGui SDL3 platform initialization failed.\\n";
        Shutdown();
        return false;
    }
    if (!ImGui_ImplSDLRenderer3_Init(m_renderer))
    {
        std::cerr << "EMBER Editor: ImGui SDL renderer initialization failed.\\n";
        Shutdown();
        return false;
    }

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
        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
            SaveScene();
        }
        if (ImGui::MenuItem("Load Scene"))
        {
            LoadScene();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit"))
        {
            m_isRunning = false;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Entity"))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            CreateEntity();
        }
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
        if (ImGui::SmallButton("+"))
        {
            CreateEntity();
        }
        ImGui::Separator();

        m_scene.ForEachEntity([this](Entity& entity)
        {
            const bool selected = entity.GetId() == m_selectedEntity;
            char label[160]{};
            std::snprintf(label, sizeof(label), "%s##%llu",
                entity.GetName().c_str(),
                static_cast<unsigned long long>(entity.GetId()));
            if (ImGui::Selectable(label, selected))
            {
                m_selectedEntity = entity.GetId();
            }
        });

        if (m_scene.IsEmpty())
        {
            ImGui::TextDisabled("No entities. Press + to create one.");
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
        Entity* entity = m_scene.FindEntity(m_selectedEntity);
        if (entity == nullptr)
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
            }

            bool active = entity->IsActive();
            if (ImGui::Checkbox("Active", &active))
            {
                entity->SetActive(active);
            }

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
                render.red = static_cast<std::uint8_t>(color[0] < 0 ? 0 : color[0] > 255 ? 255 : color[0]);
                render.green = static_cast<std::uint8_t>(color[1] < 0 ? 0 : color[1] > 255 ? 255 : color[1]);
                render.blue = static_cast<std::uint8_t>(color[2] < 0 ? 0 : color[2] > 255 ? 255 : color[2]);
                render.alpha = static_cast<std::uint8_t>(color[3] < 0 ? 0 : color[3] > 255 ? 255 : color[3]);
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
        ImGui::TextDisabled("Asset registry integration is the next editor task.");
        ImGui::Text("Scene file: %s", m_scenePath);
        ImGui::InputText("Path", m_scenePath, sizeof(m_scenePath));
        if (ImGui::Button("Save Scene"))
        {
            SaveScene();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Scene"))
        {
            LoadScene();
        }
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
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Saved scene: %.200s", m_scenePath);
    }
    else
    {
        std::snprintf(m_statusMessage, sizeof(m_statusMessage), "Save failed: %.220s", error.c_str());
    }
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
} // namespace ember
