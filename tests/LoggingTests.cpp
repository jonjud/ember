#include "Engine/Core/Logger.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
class RecordingSink final : public ember::LogSink
{
public:
    void Write(const ember::LogEntry& entry) override { messages.push_back(entry.message); }
    std::vector<std::string> messages;
};

int g_failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        ++g_failures;
    }
}
}

int main()
{
    ember::Logger logger(3);
    Check(logger.Capacity() == 3, "Logger exposes its configured capacity");
    Check(logger.GetMinimumLevel() == ember::LogLevel::Trace,
        "Logger accepts all levels by default");

    logger.Trace("Engine", "trace");
    logger.Debug("Engine", "debug");
    logger.Info("Renderer", "ready");
    logger.Warning("Assets", "missing optional asset");
    logger.Error("Scene", "load failed");
    logger.Critical("Runtime", "fatal");

    auto entries = logger.Snapshot();
    Check(entries.size() == 3, "History is bounded to its configured capacity");
    Check(entries[0].message == "missing optional asset" &&
          entries[1].message == "load failed" &&
          entries[2].message == "fatal",
        "The bounded history retains the newest entries");

    Check(entries[0].sequence == 4 && entries[1].sequence == 5,
        "Log entries receive increasing sequence numbers");
    Check(entries[0].category == "Assets" &&
          entries[0].level == ember::LogLevel::Warning,
        "Structured category and severity are retained");
    Check(entries[0].timestamp <= entries[1].timestamp,
        "Log timestamps are chronological");
    Check(entries[0].threadId == std::this_thread::get_id(),
        "Log entries record the calling thread");
    Check(std::string(ember::ToString(ember::LogLevel::Warning)) == "Warning",
        "Severity names can be formatted");

    logger.SetMinimumLevel(ember::LogLevel::Warning);
    logger.Info("Engine", "filtered");
    logger.Warning("Engine", "visible warning");
    Check(logger.Size() == 3 &&
          logger.Snapshot().back().message == "visible warning",
        "Minimum severity filters lower-priority entries");
    logger.SetMinimumLevel(ember::LogLevel::Off);
    logger.Critical("Engine", "filtered while off");
    Check(logger.Size() == 3, "Off disables log collection");

    logger.Clear();
    Check(logger.Size() == 0 && logger.Snapshot().empty(),
        "History can be cleared");
    logger.SetMinimumLevel(ember::LogLevel::Trace);
    logger.Info("Engine", "after clear");
    Check(logger.Snapshot().front().sequence == 8,
        "Clearing history does not reset sequence numbers");

    auto recordingSink = std::make_shared<RecordingSink>();
    ember::Logger sinkLogger(4);
    sinkLogger.AddSink(recordingSink);
    sinkLogger.Info("Tests", "sink delivery");
    Check(recordingSink->messages.size() == 1 && recordingSink->messages[0] == "sink delivery",
        "Logger dispatches entries to registered sinks");
    sinkLogger.ClearSinks();
    sinkLogger.Info("Tests", "not delivered");
    Check(recordingSink->messages.size() == 1, "Logger can detach all sinks");

    const auto logPath = std::filesystem::temp_directory_path() / "ember_logging_tests.log";
    std::filesystem::remove(logPath);
    {
        auto fileSink = std::make_shared<ember::FileLogSink>(logPath);
        Check(fileSink->IsOpen(), "File sink opens its target path");
        ember::Logger fileLogger(2);
        fileLogger.AddSink(fileSink);
        fileLogger.Warning("Persistence", "file sink entry");
        fileSink->Flush();
    }
    std::ifstream logInput(logPath);
    const std::string logContents((std::istreambuf_iterator<char>(logInput)),
        std::istreambuf_iterator<char>());
    Check(logContents.find("[Warning] [Persistence]") != std::string::npos &&
          logContents.find("file sink entry") != std::string::npos,
        "File sink writes severity, category and message");
    std::filesystem::remove(logPath);

    ember::Logger disabled(0);
    auto disabledSink = std::make_shared<RecordingSink>();
    disabled.AddSink(disabledSink);
    disabled.Error("Engine", "not stored in memory but still sent to sink");
    Check(disabled.Size() == 0 && disabledSink->messages.size() == 1,
        "Zero capacity disables history but keeps configured sinks active");
    disabled.Error("Engine", "not stored");
    Check(disabled.Size() == 0, "Zero capacity disables history storage");

    ember::Logger concurrent(512);
    constexpr int threadCount = 4;
    constexpr int messagesPerThread = 100;
    std::vector<std::thread> workers;
    workers.reserve(threadCount);
    for (int thread = 0; thread < threadCount; ++thread)
    {
        workers.emplace_back([&concurrent, thread]
        {
            for (int index = 0; index < messagesPerThread; ++index)
                concurrent.Info("Worker", std::to_string(thread) + ":" + std::to_string(index));
        });
    }
    for (auto& worker : workers)
        worker.join();

    entries = concurrent.Snapshot();
    Check(entries.size() == threadCount * messagesPerThread,
        "Concurrent writers retain every entry within capacity");
    bool ordered = true;
    for (std::size_t index = 1; index < entries.size(); ++index)
        ordered = ordered && entries[index - 1].sequence < entries[index].sequence;
    Check(ordered, "Concurrent writes have a unique ordered sequence");

    if (g_failures != 0)
    {
        std::cerr << g_failures << " logging test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All logging tests passed.\n";
    return EXIT_SUCCESS;
}
