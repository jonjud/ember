#pragma once

#include "Engine/Scene/RenderComponent2D.hpp"
#include "Engine/Scene/Transform2D.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace ember
{
using EntityId = std::uint64_t;

class Scene;

// An entity is a lightweight scene object with identity, transform, and render data.
class Entity
{
public:
    [[nodiscard]] EntityId GetId() const { return m_id; }

    [[nodiscard]] const std::string& GetName() const { return m_name; }
    void SetName(std::string name) { m_name = std::move(name); }

    [[nodiscard]] Transform2D& GetTransform() { return m_transform; }
    [[nodiscard]] const Transform2D& GetTransform() const { return m_transform; }

    [[nodiscard]] RenderComponent2D& GetRenderComponent() { return m_renderComponent; }
    [[nodiscard]] const RenderComponent2D& GetRenderComponent() const { return m_renderComponent; }

    [[nodiscard]] bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }

private:
    friend class Scene;

    Entity(EntityId id, std::string name)
        : m_id(id), m_name(std::move(name))
    {
    }

    EntityId m_id = 0;
    std::string m_name;
    Transform2D m_transform{};
    RenderComponent2D m_renderComponent{};
    bool m_active = true;
};
} // namespace ember
