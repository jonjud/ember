#pragma once

#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Resources/AssetMetadata.hpp"
#include "Engine/Resources/TextureHandle.hpp"
#include "Engine/Scene/Transform2D.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

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
    void DrawFilledRect(
        const Transform2D& transform,
        const Camera2D& camera,
        const Vector2& size,
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue,
        std::uint8_t alpha = 255);
    [[nodiscard]] TextureHandle LoadTexture(const std::string& filePath, const TextureImportSettings& settings = {});
    void DestroyTexture(TextureHandle texture);
    void DrawTexture(TextureHandle texture, float x, float y, float width, float height);
    void DrawTexture(
        TextureHandle texture,
        const Transform2D& transform,
        const Camera2D& camera,
        const Vector2& size,
        const Vector2& sourcePosition,
        const Vector2& sourceSize,
        std::uint8_t red = 255,
        std::uint8_t green = 255,
        std::uint8_t blue = 255,
        std::uint8_t alpha = 255);
    void DrawTexture(
        TextureHandle texture,
        const Transform2D& transform,
        const Camera2D& camera,
        const Vector2& size,
        std::uint8_t red = 255,
        std::uint8_t green = 255,
        std::uint8_t blue = 255,
        std::uint8_t alpha = 255);
    void Present();

private:
    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<std::uint32_t, SDL_Texture*> m_textures;
    std::uint32_t m_nextTextureId = 1;
};
} // namespace ember
