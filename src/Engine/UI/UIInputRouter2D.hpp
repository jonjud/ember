#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/UI/UILayout2D.hpp"

#include <map>
#include <vector>

namespace ember
{
struct UIButtonInteractionState2D
{
    bool hovered = false;
    bool pressed = false;
    bool clicked = false;
};

struct UIButtonClickedEvent2D
{
    EntityId entity = 0;
};

class UIInputRouter2D
{
public:
    // Pointer coordinates are relative to the viewport's top-left corner.
    // Canvas layout conversion also accounts for aspect-ratio letterboxing.
    void Update(
        const Scene& scene,
        const UICanvas2D* canvas,
        Vector2 viewportSize,
        Vector2 pointerPosition,
        bool pointerDown,
        bool pointerPressed,
        bool pointerReleased)
    {
        m_states.clear();
        m_events.clear();

        UICanvasLayout2D layout;
        if (canvas == nullptr || !ResolveUICanvasLayout(*canvas, viewportSize, layout))
        {
            m_capturedEntity = 0;
            return;
        }

        const Vector2 canvasPointer{
            (pointerPosition.x - layout.origin.x) / layout.scale,
            (pointerPosition.y - layout.origin.y) / layout.scale
        };

        EntityId hoveredEntity = 0;
        const std::vector<EntityId> entityIds = scene.GetEntityIds();
        // Rendering follows ascending EntityId order; reverse hit testing picks
        // the visually topmost button when controls overlap.
        for (auto it = entityIds.rbegin(); it != entityIds.rend(); ++it)
        {
            const Entity* entity = scene.FindEntity(*it);
            if (entity == nullptr || !entity->IsActive())
                continue;

            const auto* button = entity->GetComponent<UIButton2D>();
            if (button == nullptr || !button->enabled || !button->interactable ||
                !button->IsConfigurationValid())
                continue;

            const bool inside =
                canvasPointer.x >= button->position.x &&
                canvasPointer.y >= button->position.y &&
                canvasPointer.x < button->position.x + button->size.x &&
                canvasPointer.y < button->position.y + button->size.y;
            if (inside)
            {
                hoveredEntity = entity->GetId();
                break;
            }
        }

        if (m_capturedEntity != 0)
        {
            const Entity* captured = scene.FindEntity(m_capturedEntity);
            const auto* button = captured != nullptr
                ? captured->GetComponent<UIButton2D>() : nullptr;
            if (captured == nullptr || !captured->IsActive() || button == nullptr ||
                !button->enabled || !button->interactable || !button->IsConfigurationValid())
                m_capturedEntity = 0;
        }

        if (pointerPressed && m_capturedEntity == 0 && hoveredEntity != 0)
            m_capturedEntity = hoveredEntity;

        if (pointerReleased && m_capturedEntity != 0 && hoveredEntity == m_capturedEntity)
            m_events.push_back({m_capturedEntity});

        for (const EntityId id : entityIds)
        {
            const Entity* entity = scene.FindEntity(id);
            if (entity == nullptr)
                continue;
            const auto* button = entity->GetComponent<UIButton2D>();
            if (button == nullptr)
                continue;

            UIButtonInteractionState2D state;
            const bool usable = entity->IsActive() && button->enabled &&
                button->interactable && button->IsConfigurationValid();
            if (usable)
            {
                state.hovered = hoveredEntity == id;
                state.pressed = m_capturedEntity == id && pointerDown;
                state.clicked = pointerReleased && m_capturedEntity == id &&
                    hoveredEntity == id;
            }
            m_states.emplace(id, state);
        }

        if (pointerReleased || (!pointerDown && !pointerPressed))
            m_capturedEntity = 0;
    }

    [[nodiscard]] const UIButtonInteractionState2D& GetState(EntityId entity) const noexcept
    {
        const auto it = m_states.find(entity);
        return it != m_states.end() ? it->second : EmptyState();
    }

    [[nodiscard]] const std::vector<UIButtonClickedEvent2D>& GetEvents() const noexcept
    {
        return m_events;
    }

    void Reset() noexcept
    {
        m_states.clear();
        m_events.clear();
        m_capturedEntity = 0;
    }

private:
    [[nodiscard]] static const UIButtonInteractionState2D& EmptyState() noexcept
    {
        static const UIButtonInteractionState2D empty{};
        return empty;
    }

    std::map<EntityId, UIButtonInteractionState2D> m_states;
    std::vector<UIButtonClickedEvent2D> m_events;
    EntityId m_capturedEntity = 0;
};
} // namespace ember
