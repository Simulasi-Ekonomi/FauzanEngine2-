#pragma once
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <android/log.h>

namespace NeoEngine {

enum class LogLevel {
    Info,
    Warning,
    Error,
    Fatal
};

enum class LogCategory {
    Core,
    Rendering,
    Physics,
    AI,
    Network,
    UI,
    Audio,
    Gameplay
};

struct LogMessage {
    LogLevel level;
    LogCategory category;
    std::string message;
    std::string timestamp;
};

class Logger {
public:
    static void Init();
    static void Shutdown();
    static void Log(LogLevel level, LogCategory category, const std::string& msg);

private:
    static void WorkerThread();
    static std::string LevelToString(LogLevel level);
    static std::string CategoryToString(LogCategory cat);
    static int LevelToAndroid(LogLevel level);

    static std::queue<LogMessage> queue;
    static std::mutex mutex;
    static std::condition_variable cv;
    static std::thread workerThread;
    static std::atomic<bool> running;
};

} // namespace NeoEngine
