#pragma once

#include "Engine/Scene/Entity.hpp"

#include <algorithm>
#include <vector>

namespace ember
{
// Editor-side multi-selection state. The primary ID is the active Inspector target.
class EntitySelection
{
public:
    [[nodiscard]] bool Contains(EntityId id) const
    {
        return std::find(m_ids.begin(), m_ids.end(), id) != m_ids.end();
    }

    [[nodiscard]] bool Empty() const noexcept { return m_ids.empty(); }
    [[nodiscard]] std::size_t Size() const noexcept { return m_ids.size(); }
    [[nodiscard]] EntityId Primary() const noexcept { return m_primary; }
    [[nodiscard]] EntityId Anchor() const noexcept { return m_anchor; }
    [[nodiscard]] const std::vector<EntityId>& GetIds() const noexcept { return m_ids; }

    void Clear() noexcept
    {
        m_ids.clear();
        m_primary = 0;
        m_anchor = 0;
    }

    void SelectOnly(EntityId id)
    {
        Clear();
        if (id == 0) return;
        m_ids.push_back(id);
        m_primary = id;
        m_anchor = id;
    }

    void Toggle(EntityId id)
    {
        if (id == 0) return;
        const auto it = std::find(m_ids.begin(), m_ids.end(), id);
        if (it == m_ids.end())
        {
            m_ids.push_back(id);
            m_primary = id;
        }
        else
        {
            m_ids.erase(it);
            m_primary = m_ids.empty() ? 0 : m_ids.back();
        }
        m_anchor = id;
    }

    void SelectRange(const std::vector<EntityId>& orderedIds, EntityId id, bool additive)
    {
        const auto anchorIt = std::find(orderedIds.begin(), orderedIds.end(), m_anchor);
        const auto targetIt = std::find(orderedIds.begin(), orderedIds.end(), id);
        if (targetIt == orderedIds.end())
        {
            if (!additive) SelectOnly(id);
            else Toggle(id);
            return;
        }
        if (anchorIt == orderedIds.end())
        {
            SelectOnly(id);
            return;
        }

        if (!additive) m_ids.clear();
        const auto first = std::min(anchorIt, targetIt);
        const auto last = std::max(anchorIt, targetIt);
        for (auto it = first; it != std::next(last); ++it)
        {
            if (*it != 0 && !Contains(*it)) m_ids.push_back(*it);
        }
        m_primary = id;
    }

    void SelectAll(const std::vector<EntityId>& orderedIds)
    {
        m_ids.clear();
        for (EntityId id : orderedIds)
            if (id != 0 && !Contains(id)) m_ids.push_back(id);
        m_primary = m_ids.empty() ? 0 : m_ids.back();
        m_anchor = m_ids.empty() ? 0 : m_ids.front();
    }

    template <typename IsAlive>
    void Prune(IsAlive&& isAlive)
    {
        std::erase_if(m_ids, [&isAlive](EntityId id) { return !isAlive(id); });
        if (!Contains(m_primary)) m_primary = m_ids.empty() ? 0 : m_ids.back();
        if (!Contains(m_anchor)) m_anchor = m_primary;
    }

private:
    std::vector<EntityId> m_ids;
    EntityId m_primary = 0;
    EntityId m_anchor = 0;
};
} // namespace ember
