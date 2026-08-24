#include "Logger.h"
#include <iostream>
#include <fstream>
namespace NeoEngine {
    std::queue<LogMessage> Logger::queue;
    std::mutex Logger::mutex;
    std::condition_variable Logger::cv;
    std::thread Logger::workerThread;
    std::atomic<bool> Logger::running = false;
    static std::ofstream logfile;
    void Logger::Init() {
        running = true;
        logfile.open("neoengine.log");
        workerThread = std::thread(Worker);
    }
    void Logger::Shutdown() {
        running = false;
        cv.notify_all();
        if (workerThread.joinable()) workerThread.join();
        logfile.close();
    }
    void Logger::Log(LogLevel level, LogCategory category, const std::string& message) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push({level, category, message});
        }
        cv.notify_one();
    }
    void Logger::Worker() {
        while (running) {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, []{ return !queue.empty() || !running; });
            while (!queue.empty()) {
                auto msg = queue.front();
                queue.pop();
                std::string output = msg.text;
                std::cout << output << std::endl;
                if (logfile.is_open()) logfile << output << std::endl;
            }
        }
    }
}
