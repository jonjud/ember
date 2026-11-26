#pragma once

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

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_isRunning = false;
};
} // namespace ember
