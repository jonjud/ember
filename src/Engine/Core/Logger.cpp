#include "Engine/Core/Logger.hpp"

#include <algorithm>
#include <utility>

namespace ember
{
const char* ToString(LogLevel level) noexcept
{
    switch (level)
    {
    case LogLevel::Trace: return "Trace";
    case LogLevel::Debug: return "Debug";
    case LogLevel::Info: return "Info";
    case LogLevel::Warning: return "Warning";
    case LogLevel::Error: return "Error";
    case LogLevel::Critical: return "Critical";
    case LogLevel::Off: return "Off";
    }
    return "Unknown";
}

Logger::Logger(std::size_t capacity) noexcept
    : m_capacity(capacity)
{
}

void Logger::Log(LogLevel level, std::string_view category, std::string_view message)
{
    std::lock_guard lock(m_mutex);
    if (m_capacity == 0 || level == LogLevel::Off ||
        level < m_minimumLevel || m_minimumLevel == LogLevel::Off)
        return;

    if (m_entries.size() == m_capacity)
        m_entries.pop_front();

    LogEntry entry;
    entry.sequence = m_nextSequence++;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.category.assign(category);
    entry.message.assign(message);
    entry.threadId = std::this_thread::get_id();
    m_entries.push_back(std::move(entry));
}

void Logger::Trace(std::string_view category, std::string_view message)
{
    Log(LogLevel::Trace, category, message);
}

void Logger::Debug(std::string_view category, std::string_view message)
{
    Log(LogLevel::Debug, category, message);
}

void Logger::Info(std::string_view category, std::string_view message)
{
    Log(LogLevel::Info, category, message);
}

void Logger::Warning(std::string_view category, std::string_view message)
{
    Log(LogLevel::Warning, category, message);
}

void Logger::Error(std::string_view category, std::string_view message)
{
    Log(LogLevel::Error, category, message);
}

void Logger::Critical(std::string_view category, std::string_view message)
{
    Log(LogLevel::Critical, category, message);
}

void Logger::SetMinimumLevel(LogLevel level) noexcept
{
    std::lock_guard lock(m_mutex);
    m_minimumLevel = level;
}

LogLevel Logger::GetMinimumLevel() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_minimumLevel;
}

std::vector<LogEntry> Logger::Snapshot() const
{
    std::lock_guard lock(m_mutex);
    return {m_entries.begin(), m_entries.end()};
}

std::size_t Logger::Size() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_entries.size();
}

std::size_t Logger::Capacity() const noexcept
{
    return m_capacity;
}

void Logger::Clear() noexcept
{
    std::lock_guard lock(m_mutex);
    m_entries.clear();
}
} // namespace ember
