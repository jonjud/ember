#pragma once

#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;

namespace ember
{
class Renderer2D
{
public:
    Renderer2D() = default;
    ~Renderer2D();

    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;

    [[nodiscard]] bool Initialize(SDL_Window* window);
    void Shutdown();

    void Clear(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255);
    void DrawFilledRect(
        float x,
        float y,
        float width,
        float height,
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue,
        std::uint8_t alpha = 255);
    void Present();

private:
    SDL_Renderer* m_renderer = nullptr;
};
} // namespace ember
