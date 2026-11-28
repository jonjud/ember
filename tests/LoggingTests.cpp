#include "Engine/Core/Logger.hpp"
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#include <windows.h>
#endif
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
    std::cerr << "FileSink: resolving temp directory" << std::endl;
    std::error_code error;
    const auto tempDirectory = std::filesystem::temp_directory_path(error);
    Check(!error, "resolve temp directory");
    if (error) return;

    const auto path = tempDirectory / "ember_logging_tests.log";
    std::cerr << "FileSink: removing stale file" << std::endl;
    error.clear();
    std::filesystem::remove(path, error);
    Check(!error, "remove stale log file");
    if (error) return;

    {
        std::cerr << "FileSink: constructing sink" << std::endl;
        auto sink = std::make_shared<ember::FileLogSink>(path);
        Check(sink->IsOpen(), "file sink opens");
        if (!sink->IsOpen()) return;

        std::cerr << "FileSink: writing entry" << std::endl;
        ember::Logger fileLogger(2);
        fileLogger.AddSink(sink);
        fileLogger.Warning("Persistence", "file sink entry");

        std::cerr << "FileSink: flushing" << std::endl;
        sink->Flush();
        std::cerr << "FileSink: leaving sink scope" << std::endl;
    }

    std::cerr << "FileSink: reading persisted file" << std::endl;
    std::ifstream input(path);
    Check(input.is_open(), "open persisted log file");
    if (input.is_open()) {
        const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        Check(contents.find("[Warning] [Persistence]") != std::string::npos &&
              contents.find("file sink entry") != std::string::npos, "file sink contents");
    }
    input.close();

    std::cerr << "FileSink: removing test file" << std::endl;
    error.clear();
    std::filesystem::remove(path, error);
    Check(!error, "remove test log file");
    std::cerr << "FileSink: completed" << std::endl;
}
void FileSinkRotationTest() {
    std::error_code error;
    const auto directory = std::filesystem::temp_directory_path(error);
    Check(!error, "resolve temp directory for rotation");
    if (error) return;
    const auto path = directory / "ember_logging_rotation_tests.log";
    const auto backup = std::filesystem::path(path.string() + ".1");
    std::filesystem::remove(path, error); error.clear();
    std::filesystem::remove(backup, error); error.clear();
    {
        auto sink = std::make_shared<ember::FileLogSink>(path, 1);
        Check(sink->IsOpen(), "rotation sink opens");
        ember::Logger logger(4);
        logger.AddSink(sink);
        logger.Info("Rotation", "first entry");
        logger.Info("Rotation", "second entry");
        sink->Flush();
    }
    std::ifstream current(path);
    std::ifstream previous(backup);
    const std::string currentText((std::istreambuf_iterator<char>(current)), {});
    const std::string previousText((std::istreambuf_iterator<char>(previous)), {});
    Check(currentText.find("second entry") != std::string::npos, "new log remains current after rotation");
    Check(previousText.find("first entry") != std::string::npos, "previous log is retained as one backup");
    current.close(); previous.close();
    std::filesystem::remove(path, error); error.clear();
    std::filesystem::remove(backup, error);
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
#ifdef _WIN32
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    const std::string stage = argc > 1 ? argv[1] : "all";
    if (stage == "history" || stage == "all") History();
    else if (stage == "recording") RecordingSinkTest();
    else if (stage == "file") FileSinkTest();
    else if (stage == "zero-capacity") ZeroCapacitySinkTest();
    else if (stage == "rotation") FileSinkRotationTest();
    else if (stage == "concurrency") Concurrency();
    else return EXIT_FAILURE;
    if (failures) { std::cerr << failures << " logger test(s) failed\n"; return EXIT_FAILURE; }
    std::cout << "Logger " << stage << " tests passed\n";
    return EXIT_SUCCESS;
}
