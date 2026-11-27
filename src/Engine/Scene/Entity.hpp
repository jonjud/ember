#pragma once

#include "Engine/Scene/Component.hpp"
#include "Engine/Scene/RenderComponent2D.hpp"
#include "Engine/Scene/Transform2D.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ember
{
using EntityId = std::uint64_t;

class Scene;
class SceneSerializer;

struct MovementComponent2D
{
    bool enabled = false;
    float speed = 240.0F;
};

// An entity is a lightweight scene object with identity, transform, render data,
// and an extensible collection of runtime behavior components.
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
    [[nodiscard]] MovementComponent2D& GetMovementComponent() { return m_movementComponent; }
    [[nodiscard]] const MovementComponent2D& GetMovementComponent() const { return m_movementComponent; }

    template <typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from ember::Component");
        if (m_componentsStarted)
        {
            throw std::logic_error("Components cannot be added while their Scene is running.");
        }
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T& result = *component;
        m_components.push_back(std::move(component));
        return result;
    }

    Component& AddComponent(std::unique_ptr<Component> component)
    {
        if (m_componentsStarted)
            throw std::logic_error("Components cannot be added while their Scene is running.");
        if (!component) throw std::invalid_argument("Cannot add a null component.");
        Component& result = *component;
        m_components.push_back(std::move(component));
        return result;
    }

    [[nodiscard]] std::vector<std::unique_ptr<Component>>& GetComponents() { return m_components; }
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& GetComponents() const { return m_components; }

    bool RemoveComponent(const Component* component)
    {
        if (m_componentsStarted)
            throw std::logic_error("Components cannot be removed while their Scene is running.");
        const auto it = std::find_if(m_components.begin(), m_components.end(),
            [component](const std::unique_ptr<Component>& value) { return value.get() == component; });
        if (it == m_components.end()) return false;
        m_components.erase(it);
        return true;
    }

    template <typename T>
    [[nodiscard]] T* GetComponent()
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from ember::Component");
        for (const auto& component : m_components)
        {
            if (auto* result = dynamic_cast<T*>(component.get())) return result;
        }
        return nullptr;
    }

    template <typename T>
    [[nodiscard]] const T* GetComponent() const
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from ember::Component");
        for (const auto& component : m_components)
        {
            if (const auto* result = dynamic_cast<const T*>(component.get())) return result;
        }
        return nullptr;
    }

    template <typename T>
    [[nodiscard]] bool HasComponent() const { return GetComponent<T>() != nullptr; }

    template <typename T>
    bool RemoveComponent()
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from ember::Component");
        if (m_componentsStarted)
        {
            throw std::logic_error("Components cannot be removed while their Scene is running.");
        }
        const auto it = std::find_if(m_components.begin(), m_components.end(),
            [](const std::unique_ptr<Component>& component)
            {
                return dynamic_cast<T*>(component.get()) != nullptr;
            });
        if (it == m_components.end()) return false;
        m_components.erase(it);
        return true;
    }

    [[nodiscard]] bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }

private:
    friend class Scene;
    friend class SceneSerializer;

    Entity(EntityId id, std::string name) : m_id(id), m_name(std::move(name)) {}

    void CopyComponentsTo(Entity& destination) const
    {
        destination.m_components.reserve(m_components.size());
        for (const auto& component : m_components)
        {
            auto copy = component->Clone();
            if (!copy) throw std::logic_error("Component::Clone returned a null component.");
            destination.m_components.push_back(std::move(copy));
        }
    }

    void StartComponents() noexcept
    {
        m_componentsStarted = true;
        for (const auto& component : m_components) component->OnCreate(*this);
    }

    void UpdateComponents(float deltaSeconds) noexcept
    {
        for (const auto& component : m_components) component->OnUpdate(*this, deltaSeconds);
    }

    void DestroyComponents() noexcept
    {
        if (!m_componentsStarted) return;
        for (auto it = m_components.rbegin(); it != m_components.rend(); ++it)
            (*it)->OnDestroy(*this);
        m_componentsStarted = false;
    }

    EntityId m_id = 0;
    std::string m_name;
    Transform2D m_transform{};
    RenderComponent2D m_renderComponent{};
    MovementComponent2D m_movementComponent{};
    std::vector<std::unique_ptr<Component>> m_components;
    bool m_componentsStarted = false;
    bool m_active = true;
};
} // namespace ember
