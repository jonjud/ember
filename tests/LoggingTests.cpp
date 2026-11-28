#include "Engine/Core/Logger.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {
class RecordingSink final : public ember::LogSink {
public:
    void Write(const ember::LogEntry& entry) override { messages.push_back(entry.message); }
    std::vector<std::string> messages;
};
int failures = 0;
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; ++failures; }
}
void History() {
    ember::Logger logger(3);
    Check(logger.Capacity() == 3, "configured capacity");
    Check(logger.GetMinimumLevel() == ember::LogLevel::Trace, "default level");
    logger.Trace("Engine", "trace");
    logger.Debug("Engine", "debug");
    logger.Info("Renderer", "ready");
    logger.Warning("Assets", "missing optional asset");
    logger.Error("Scene", "load failed");
    logger.Critical("Runtime", "fatal");
    auto entries = logger.Snapshot();
    Check(entries.size() == 3, "bounded history");
    Check(entries[0].message == "missing optional asset" && entries[1].message == "load failed" && entries[2].message == "fatal", "newest entries retained");
    Check(entries[0].sequence == 4 && entries[1].sequence == 5, "increasing sequence");
    Check(entries[0].category == "Assets" && entries[0].level == ember::LogLevel::Warning, "structured fields");
    Check(entries[0].timestamp <= entries[1].timestamp, "chronological timestamps");
    Check(entries[0].threadId == std::this_thread::get_id(), "thread metadata");
    Check(std::string(ember::ToString(ember::LogLevel::Warning)) == "Warning", "severity formatting");
    logger.SetMinimumLevel(ember::LogLevel::Warning);
    logger.Info("Engine", "filtered");
    logger.Warning("Engine", "visible warning");
    Check(logger.Size() == 3 && logger.Snapshot().back().message == "visible warning", "minimum level filter");
    logger.SetMinimumLevel(ember::LogLevel::Off);
    logger.Critical("Engine", "filtered while off");
    Check(logger.Size() == 3, "off disables collection");
    logger.Clear();
    Check(logger.Size() == 0 && logger.Snapshot().empty(), "clear history");
    logger.SetMinimumLevel(ember::LogLevel::Trace);
    logger.Info("Engine", "after clear");
    Check(logger.Snapshot().front().sequence == 8, "clear preserves sequence");
}
void RecordingSinkTest() {
    auto recording = std::make_shared<RecordingSink>();
    ember::Logger logger(4);
    logger.AddSink(recording);
    logger.Info("Tests", "sink delivery");
    Check(recording->messages.size() == 1 && recording->messages[0] == "sink delivery", "sink delivery");
    logger.ClearSinks();
    logger.Info("Tests", "not delivered");
    Check(recording->messages.size() == 1, "clear sinks");
}
void FileSinkTest() {
    const auto path = std::filesystem::temp_directory_path() / "ember_logging_tests.log";
    std::filesystem::remove(path);
    {
        auto sink = std::make_shared<ember::FileLogSink>(path);
        Check(sink->IsOpen(), "file sink opens");
        ember::Logger fileLogger(2);
        fileLogger.AddSink(sink);
        fileLogger.Warning("Persistence", "file sink entry");
        sink->Flush();
    }
    std::ifstream input(path);
    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    Check(contents.find("[Warning] [Persistence]") != std::string::npos && contents.find("file sink entry") != std::string::npos, "file sink contents");
    std::filesystem::remove(path);
}
void ZeroCapacitySinkTest() {
    ember::Logger disabled(0);
    auto disabledSink = std::make_shared<RecordingSink>();
    disabled.AddSink(disabledSink);
    disabled.Error("Engine", "not stored in memory but sent to sink");
    Check(disabled.Size() == 0 && disabledSink->messages.size() == 1, "zero capacity still dispatches");
    disabled.Error("Engine", "not stored");
    Check(disabled.Size() == 0, "zero capacity history");
}
void Concurrency() {
    ember::Logger logger(512);
    constexpr int threadCount = 4;
    constexpr int messagesPerThread = 100;
    std::vector<std::thread> workers;
    workers.reserve(threadCount);
    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&logger, t] {
            for (int i = 0; i < messagesPerThread; ++i)
                logger.Info("Worker", std::to_string(t) + ":" + std::to_string(i));
        });
    }
    for (auto& worker : workers) worker.join();
    const auto entries = logger.Snapshot();
    Check(entries.size() == threadCount * messagesPerThread, "concurrent entries retained");
    bool ordered = true;
    for (std::size_t i = 1; i < entries.size(); ++i)
        ordered = ordered && entries[i - 1].sequence < entries[i].sequence;
    Check(ordered, "concurrent sequence order");
}
}
int main(int argc, char** argv) {
    const std::string stage = argc > 1 ? argv[1] : "all";
    if (stage == "history" || stage == "all") History();
    else if (stage == "recording") RecordingSinkTest();
    else if (stage == "file") FileSinkTest();
    else if (stage == "zero-capacity") ZeroCapacitySinkTest();
    else if (stage == "concurrency") Concurrency();
    else return EXIT_FAILURE;
    if (failures) { std::cerr << failures << " logger test(s) failed\n"; return EXIT_FAILURE; }
    std::cout << "Logger " << stage << " tests passed\n";
    return EXIT_SUCCESS;
}
