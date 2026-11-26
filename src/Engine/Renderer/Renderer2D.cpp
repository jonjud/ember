#include "Engine/Renderer/Renderer2D.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cmath>
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

    if (m_renderer != nullptr)
    {
        Shutdown();
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
    for (const auto& [id, texture] : m_textures)
    {
        (void)id;
        SDL_DestroyTexture(texture);
    }
    m_textures.clear();
    m_nextTextureId = 1;

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

void Renderer2D::DrawFilledRect(
    const Transform2D& transform,
    const Camera2D& camera,
    const Vector2& size,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha)
{
    if (m_renderer == nullptr)
    {
        return;
    }

    const Vector2 scaledSize{
        size.x * transform.scale.x,
        size.y * transform.scale.y
    };
    const Vector2 center{
        transform.position.x + scaledSize.x * 0.5F,
        transform.position.y + scaledSize.y * 0.5F
    };

    const float halfWidth = scaledSize.x * 0.5F;
    const float halfHeight = scaledSize.y * 0.5F;
    const float cosine = std::cos(transform.rotation);
    const float sine = std::sin(transform.rotation);
    const std::array<Vector2, 4> localCorners{
        Vector2{-halfWidth, -halfHeight},
        Vector2{ halfWidth, -halfHeight},
        Vector2{ halfWidth,  halfHeight},
        Vector2{-halfWidth,  halfHeight}
    };

    const SDL_FColor color{
        static_cast<float>(red) / 255.0F,
        static_cast<float>(green) / 255.0F,
        static_cast<float>(blue) / 255.0F,
        static_cast<float>(alpha) / 255.0F
    };
    std::array<SDL_Vertex, 4> vertices{};

    for (std::size_t i = 0; i < localCorners.size(); ++i)
    {
        const Vector2& corner = localCorners[i];
        const Vector2 worldCorner{
            center.x + corner.x * cosine - corner.y * sine,
            center.y + corner.x * sine + corner.y * cosine
        };
        const Vector2 screenCorner = camera.WorldToScreen(worldCorner);
        vertices[i] = SDL_Vertex{
            {screenCorner.x, screenCorner.y},
            color,
            {0.0F, 0.0F}
        };
    }

    constexpr int indices[]{0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(m_renderer, nullptr, vertices.data(),
                       static_cast<int>(vertices.size()), indices, 6);
}

TextureHandle Renderer2D::LoadTexture(const std::string& filePath)
{
    if (m_renderer == nullptr || filePath.empty())
    {
        std::cerr << "EMBER: Cannot load a texture without an initialized renderer and file path.\n";
        return {};
    }

    SDL_Surface* surface = SDL_LoadBMP(filePath.c_str());
    if (surface == nullptr)
    {
        std::cerr << "EMBER: Failed to load BMP '" << filePath << "': " << SDL_GetError() << '\n';
        return {};
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_DestroySurface(surface);

    if (texture == nullptr)
    {
        std::cerr << "EMBER: Failed to create texture from '" << filePath << "': " << SDL_GetError() << '\n';
        return {};
    }

    const std::uint32_t id = m_nextTextureId++;
    m_textures.emplace(id, texture);
    return {id};
}

void Renderer2D::DestroyTexture(TextureHandle texture)
{
    const auto iterator = m_textures.find(texture.id);
    if (iterator == m_textures.end())
    {
        return;
    }

    SDL_DestroyTexture(iterator->second);
    m_textures.erase(iterator);
}

void Renderer2D::DrawTexture(TextureHandle texture, float x, float y, float width, float height)
{
    if (m_renderer == nullptr)
    {
        return;
    }

    const auto iterator = m_textures.find(texture.id);
    if (iterator == m_textures.end())
    {
        return;
    }

    const SDL_FRect destination{x, y, width, height};
    SDL_RenderTexture(m_renderer, iterator->second, nullptr, &destination);
}

void Renderer2D::DrawTexture(
    TextureHandle texture,
    const Transform2D& transform,
    const Camera2D& camera,
    const Vector2& size,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha)
{
    if (m_renderer == nullptr)
    {
        return;
    }

    const auto iterator = m_textures.find(texture.id);
    if (iterator == m_textures.end())
    {
        return;
    }

    const Vector2 scaledSize{
        size.x * transform.scale.x,
        size.y * transform.scale.y
    };
    const Vector2 worldCenter{
        transform.position.x + scaledSize.x * 0.5F,
        transform.position.y + scaledSize.y * 0.5F
    };
    const Vector2 screenSize = camera.WorldToScreenSize(scaledSize);
    const Vector2 screenCenter = camera.WorldToScreen(worldCenter);
    const SDL_FRect destination{
        screenCenter.x - screenSize.x * 0.5F,
        screenCenter.y - screenSize.y * 0.5F,
        screenSize.x,
        screenSize.y
    };

    constexpr double kRadiansToDegrees = 180.0 / 3.14159265358979323846;
    const double screenRotation =
        static_cast<double>(transform.rotation - camera.GetRotation()) * kRadiansToDegrees;

    SDL_SetTextureColorMod(iterator->second, red, green, blue);
    SDL_SetTextureAlphaMod(iterator->second, alpha);
    SDL_RenderTextureRotated(
        m_renderer,
        iterator->second,
        nullptr,
        &destination,
        screenRotation,
        nullptr,
        SDL_FLIP_NONE);

    // Texture tint is mutable SDL state; restore the neutral tint for later draws.
    SDL_SetTextureColorMod(iterator->second, 255, 255, 255);
    SDL_SetTextureAlphaMod(iterator->second, 255);
}

void Renderer2D::Present()
{
    if (m_renderer != nullptr)
    {
        SDL_RenderPresent(m_renderer);
    }
}
} // namespace ember
