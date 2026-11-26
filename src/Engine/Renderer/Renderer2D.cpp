#include "Engine/Renderer/Renderer2D.hpp"

#include <SDL3/SDL.h>

#include <iostream>

namespace ember
{
Renderer2D::~Renderer2D()
{
    Shutdown();
}

bool Renderer2D::Initialize(SDL_Window* window)
{
    if (window == nullptr)
    {
        std::cerr << "EMBER: Renderer2D requires a valid window.\n";
        return false;
    }

    m_renderer = SDL_CreateRenderer(window, nullptr);
    if (m_renderer == nullptr)
    {
        std::cerr << "EMBER: Renderer creation failed: " << SDL_GetError() << '\n';
        return false;
    }

    // Prefer VSync to avoid an unrestricted render loop consuming a CPU core.
    if (!SDL_SetRenderVSync(m_renderer, 1))
    {
        std::cerr << "EMBER: VSync could not be enabled: " << SDL_GetError() << '\n';
    }

    return true;
}

void Renderer2D::Shutdown()
{
    if (m_renderer != nullptr)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
}

void Renderer2D::Clear(
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha)
{
    if (m_renderer == nullptr)
    {
        return;
    }

    SDL_SetRenderDrawColor(m_renderer, red, green, blue, alpha);
    SDL_RenderClear(m_renderer);
}

void Renderer2D::DrawFilledRect(
    float x,
    float y,
    float width,
    float height,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha)
{
    if (m_renderer == nullptr)
    {
        return;
    }

    const SDL_FRect rectangle{x, y, width, height};
    SDL_SetRenderDrawColor(m_renderer, red, green, blue, alpha);
    SDL_RenderFillRect(m_renderer, &rectangle);
}

void Renderer2D::Present()
{
    if (m_renderer != nullptr)
    {
        SDL_RenderPresent(m_renderer);
    }
}
} // namespace ember
