#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

namespace NeoEngine {

std::queue<LogMessage> Logger::queue;
std::mutex Logger::mutex;
std::condition_variable Logger::cv;
std::thread Logger::workerThread;
std::atomic<bool> Logger::running = false;
static std::ofstream logfile;

void Logger::Init() {
    running = true;
    logfile.open("/sdcard/FauzanEngine/Logs/engine.log", std::ios::app);
    workerThread = std::thread(WorkerThread);
}

void Logger::Shutdown() {
    running = false;
    cv.notify_all();
    if (workerThread.joinable()) {
        workerThread.join();
    }
    if (logfile.is_open()) {
        logfile.close();
    }
}

void Logger::Log(LogLevel level, LogCategory category, const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    LogMessage logMsg;
    logMsg.level = level;
    logMsg.category = category;
    logMsg.message = msg;
    logMsg.timestamp = ss.str();
    
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(logMsg);
    }
    cv.notify_one();
}

void Logger::WorkerThread() {
    while (running || !queue.empty()) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [] { return !queue.empty() || !running; });
        
        while (!queue.empty()) {
            LogMessage msg = queue.front();
            queue.pop();
            lock.unlock();
            
            // Tulis ke file
            if (logfile.is_open()) {
                logfile << msg.timestamp << " ["
                        << CategoryToString(msg.category) << "] "
                        << LevelToString(msg.level) << ": "
                        << msg.message << std::endl;
            }
            
            // Tulis ke Android logcat
            __android_log_print(
                LevelToAndroid(msg.level),
                CategoryToString(msg.category).c_str(),
                "%s", msg.message.c_str());
            
            lock.lock();
        }
    }
}

std::string Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "UNKNOWN";
}

std::string Logger::CategoryToString(LogCategory cat) {
    switch (cat) {
        case LogCategory::Core:       return "Core";
        case LogCategory::Rendering:  return "Rendering";
        case LogCategory::Physics:    return "Physics";
        case LogCategory::AI:         return "AI";
        case LogCategory::Network:    return "Network";
        case LogCategory::UI:         return "UI";
        case LogCategory::Audio:      return "Audio";
        case LogCategory::Gameplay:   return "Gameplay";
    }
    return "Unknown";
}

int Logger::LevelToAndroid(LogLevel level) {
    switch (level) {
        case LogLevel::Info:    return ANDROID_LOG_INFO;
        case LogLevel::Warning: return ANDROID_LOG_WARN;
        case LogLevel::Error:   return ANDROID_LOG_ERROR;
        case LogLevel::Fatal:   return ANDROID_LOG_FATAL;
    }
    return ANDROID_LOG_DEFAULT;
}

} // namespace NeoEngine
