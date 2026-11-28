#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <utility>

namespace ember
{
// Bounded history for canonical serialized Scene snapshots. The Editor owns
// serialization/deserialization; this class only manages immutable snapshots.
class SceneUndoHistory
{
public:
    explicit SceneUndoHistory(std::size_t maxEntries = 100,
        std::size_t maxBytes = 16U * 1024U * 1024U)
        : m_maxEntries(maxEntries), m_maxBytes(maxBytes) {}

    void Reset(std::string snapshot)
    {
        m_entries.clear();
        m_cursor = 0;
        m_bytes = 0;
        if (m_maxEntries == 0 || snapshot.size() > m_maxBytes) return;
        m_bytes = snapshot.size();
        m_entries.push_back(std::move(snapshot));
    }

    [[nodiscard]] bool Record(std::string snapshot)
    {
        if (m_entries.empty())
        {
            Reset(std::move(snapshot));
            return !m_entries.empty();
        }
        if (snapshot == m_entries[m_cursor]) return false;
        if (m_maxEntries == 0 || snapshot.size() > m_maxBytes) return false;

        while (m_entries.size() > m_cursor + 1)
        {
            m_bytes -= m_entries.back().size();
            m_entries.pop_back();
        }

        m_bytes += snapshot.size();
        m_entries.push_back(std::move(snapshot));
        m_cursor = m_entries.size() - 1;

        while (m_entries.size() > m_maxEntries || m_bytes > m_maxBytes)
        {
            m_bytes -= m_entries.front().size();
            m_entries.pop_front();
            if (m_cursor > 0) --m_cursor;
        }
        return true;
    }

    [[nodiscard]] bool CanUndo() const noexcept { return !m_entries.empty() && m_cursor > 0; }
    [[nodiscard]] bool CanRedo() const noexcept
    {
        return !m_entries.empty() && m_cursor + 1 < m_entries.size();
    }

    [[nodiscard]] const std::string* Undo() noexcept
    {
        if (!CanUndo()) return nullptr;
        --m_cursor;
        return &m_entries[m_cursor];
    }

    [[nodiscard]] const std::string* Redo() noexcept
    {
        if (!CanRedo()) return nullptr;
        ++m_cursor;
        return &m_entries[m_cursor];
    }

    [[nodiscard]] std::size_t GetEntryCount() const noexcept { return m_entries.size(); }
    [[nodiscard]] std::size_t GetCursor() const noexcept { return m_cursor; }
    [[nodiscard]] std::size_t GetBytesUsed() const noexcept { return m_bytes; }

private:
    std::deque<std::string> m_entries;
    std::size_t m_cursor = 0;
    std::size_t m_bytes = 0;
    std::size_t m_maxEntries = 100;
    std::size_t m_maxBytes = 16U * 1024U * 1024U;
};
} // namespace ember
