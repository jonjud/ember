#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace ember
{
struct TextureHandle
{
    std::uint32_t id = 0;

    [[nodiscard]] explicit operator bool() const { return id != 0; }
};

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
    [[nodiscard]] TextureHandle LoadTexture(const std::string& filePath);
    void DestroyTexture(TextureHandle texture);
    void DrawTexture(TextureHandle texture, float x, float y, float width, float height);
    void Present();

private:
    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<std::uint32_t, SDL_Texture*> m_textures;
    std::uint32_t m_nextTextureId = 1;
};
} // namespace ember
