#include "Engine/Core/Logger.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>

namespace ember
{
namespace
{
std::string FormatLogEntry(const LogEntry& entry)
{
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()).count();
    std::ostringstream stream;
    stream << '[' << milliseconds << "] [" << ToString(entry.level) << "] ["
           << entry.category << "] [thread " << entry.threadId << "] " << entry.message;
    return stream.str();
}
}

void ConsoleLogSink::Write(const LogEntry& entry)
{
    std::clog << FormatLogEntry(entry) << '\n';
}

FileLogSink::FileLogSink(const std::filesystem::path& path)
    : m_stream(path, std::ios::out | std::ios::app)
{
}

bool FileLogSink::IsOpen() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_stream.is_open() && m_stream.good();
}

void FileLogSink::Flush()
{
    std::lock_guard lock(m_mutex);
    if (m_stream.is_open()) m_stream.flush();
}

void FileLogSink::Write(const LogEntry& entry)
{
    std::lock_guard lock(m_mutex);
    if (!m_stream.is_open() || !m_stream.good()) return;
    m_stream << FormatLogEntry(entry) << '\n';
    m_stream.flush();
}

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
    LogEntry entry;
    std::vector<std::shared_ptr<LogSink>> sinks;
    {
        std::lock_guard lock(m_mutex);
        if (level == LogLevel::Off || level < m_minimumLevel ||
            m_minimumLevel == LogLevel::Off)
            return;

        entry.sequence = m_nextSequence++;
        entry.timestamp = std::chrono::system_clock::now();
        entry.level = level;
        entry.category.assign(category);
        entry.message.assign(message);
        entry.threadId = std::this_thread::get_id();

        if (m_capacity != 0)
        {
            if (m_entries.size() == m_capacity)
                m_entries.pop_front();
            m_entries.push_back(entry);
        }
        sinks = m_sinks;
    }

    // Invoke sinks outside the history lock: slow I/O must not block snapshots.
    // Logging must never propagate sink exceptions into Engine callers.
    for (const auto& sink : sinks)
    {
        if (!sink) continue;
        try { sink->Write(entry); }
        catch (...) { /* A broken sink must not break the Engine. */ }
    }
}

void Logger::AddSink(std::shared_ptr<LogSink> sink)
{
    if (!sink) return;
    std::lock_guard lock(m_mutex);
    m_sinks.push_back(std::move(sink));
}

void Logger::ClearSinks()
{
    std::lock_guard lock(m_mutex);
    m_sinks.clear();
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
