#pragma once
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>

namespace NeoEngine {

enum class LogLevel {
    Info,
    Warning,
    Error,
    Fatal
};

enum class LogChannel {
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
    LogChannel channel;
    std::string message;
    std::string timestamp;
};

class Log {
public:
    static void Init();
    static void Shutdown();
    static void Write(LogLevel level, LogChannel channel, const std::string& msg);
    
    static void SetLogFile(const std::string& path);
    static void SetMinimumLevel(LogLevel level) { s_MinLevel = level; }
    
private:
    static void WorkerThread();
    static std::string LevelToString(LogLevel level);
    static std::string ChannelToString(LogChannel channel);
    static std::string GetTimestamp();
    
    static std::queue<LogMessage> s_Queue;
    static std::mutex s_Mutex;
    static std::condition_variable s_CV;
    static std::thread s_Worker;
    static std::atomic<bool> s_Running;
    static std::ofstream s_LogFile;
    static LogLevel s_MinLevel;
};

} // namespace NeoEngine
