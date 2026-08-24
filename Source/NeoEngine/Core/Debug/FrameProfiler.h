#pragma once
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <sstream>

namespace NeoEngine {

class FrameProfiler {
public:
    static void BeginFrame();
    static void BeginSection(const std::string& name);
    static void EndSection(const std::string& name);
    static double EndFrame();
    static std::string GetReport();

private:
    static std::chrono::high_resolution_clock::time_point frameStart;
    static std::unordered_map<std::string, double> sectionTimes;
    static std::vector<std::string> sectionOrder;
    static std::mutex lock;
};

} // namespace NeoEngine
