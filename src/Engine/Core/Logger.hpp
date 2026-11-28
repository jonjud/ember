#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace ember
{
enum class LogLevel : std::uint8_t
{
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Off
};

[[nodiscard]] const char* ToString(LogLevel level) noexcept;

struct LogEntry
{
    std::uint64_t sequence = 0;
    std::chrono::system_clock::time_point timestamp{};
    LogLevel level = LogLevel::Info;
    std::string category;
    std::string message;
    std::thread::id threadId{};
};

class LogSink
{
public:
    virtual ~LogSink() = default;
    virtual void Write(const LogEntry& entry) = 0;
};

class ConsoleLogSink final : public LogSink
{
public:
    void Write(const LogEntry& entry) override;
};

class FileLogSink final : public LogSink
{
public:
    explicit FileLogSink(const std::filesystem::path& path);
    [[nodiscard]] bool IsOpen() const noexcept;
    void Flush();
    void Write(const LogEntry& entry) override;

private:
    mutable std::mutex m_mutex;
    std::ofstream m_stream;
};

// Thread-safe, bounded in-memory log history owned by the Engine.
// A capacity of zero disables storage while preserving the logging API.
class Logger
{
public:
    explicit Logger(std::size_t capacity = 1024) noexcept;

    void Log(LogLevel level, std::string_view category, std::string_view message);
    void AddSink(std::shared_ptr<LogSink> sink);
    void ClearSinks();
    void Trace(std::string_view category, std::string_view message);
    void Debug(std::string_view category, std::string_view message);
    void Info(std::string_view category, std::string_view message);
    void Warning(std::string_view category, std::string_view message);
    void Error(std::string_view category, std::string_view message);
    void Critical(std::string_view category, std::string_view message);

    void SetMinimumLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel GetMinimumLevel() const noexcept;
    [[nodiscard]] std::vector<LogEntry> Snapshot() const;
    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t Capacity() const noexcept;
    void Clear() noexcept;

private:
    const std::size_t m_capacity;
    mutable std::mutex m_mutex;
    std::deque<LogEntry> m_entries;
    std::vector<std::shared_ptr<LogSink>> m_sinks;
    LogLevel m_minimumLevel = LogLevel::Trace;
    std::uint64_t m_nextSequence = 1;
};
} // namespace ember
