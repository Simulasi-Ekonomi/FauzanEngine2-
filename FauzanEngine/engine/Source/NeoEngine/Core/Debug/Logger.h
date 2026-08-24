#pragma once
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "LogLevel.h"
#include "LogCategory.h"
namespace NeoEngine {
    struct LogMessage {
        LogLevel level;
        LogCategory category;
        std::string text;
    };
    class Logger {
    public:
        static void Init();
        static void Shutdown();
        static void Log(LogLevel level, LogCategory category, const std::string& message);
    private:
        static void Worker();
        static std::queue<LogMessage> queue;
        static std::mutex mutex;
        static std::condition_variable cv;
        static std::thread workerThread;
        static std::atomic<bool> running;
    };
}
