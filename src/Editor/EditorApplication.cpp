#include "Editor/EditorApplication.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <iostream>

namespace
{
constexpr int kWindowWidth = 1440;
constexpr int kWindowHeight = 900;
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

    m_window = SDL_CreateWindow(
        "EMBER Editor",
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_RESIZABLE);

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

    m_isRunning = true;
    std::cout << "EMBER Editor initialized with Dear ImGui.\n";
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

    ImGui::Begin("EMBER Editor");
    ImGui::TextUnformatted("Editor foundation");
    ImGui::Separator();
    ImGui::TextUnformatted("Dear ImGui is connected to SDL3.");
    ImGui::TextUnformatted("Scene Hierarchy, Inspector, Asset Browser, and Viewport are next.");
    ImGui::End();

    ImGui::Begin("Scene");
    ImGui::TextUnformatted("No scene loaded");
    ImGui::End();

    ImGui::Begin("Inspector");
    ImGui::TextUnformatted("Select an entity to inspect its properties.");
    ImGui::End();

    ImGui::Begin("Assets");
    ImGui::TextUnformatted("Project assets will appear here.");
    ImGui::End();

    ImGui::Render();

    SDL_SetRenderDrawColor(m_renderer, 24, 27, 36, 255);
    SDL_RenderClear(m_renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
    SDL_RenderPresent(m_renderer);
}
} // namespace ember
