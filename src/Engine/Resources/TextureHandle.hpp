#pragma once

#include <cstdint>

namespace ember
{
// Renderer-owned resource identifier. A zero ID represents an empty handle.
// The handle does not own the underlying texture and must not outlive its renderer.
struct TextureHandle
{
    std::uint32_t id = 0;

    [[nodiscard]] explicit operator bool() const { return id != 0; }
};
} // namespace ember
