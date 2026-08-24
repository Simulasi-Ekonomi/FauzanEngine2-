#include "FrameProfiler.h"
#include <android/log.h>
#include <algorithm>

namespace NeoEngine {

std::chrono::high_resolution_clock::time_point FrameProfiler::frameStart;
std::unordered_map<std::string, double> FrameProfiler::sectionTimes;
std::vector<std::string> FrameProfiler::sectionOrder;
std::mutex FrameProfiler::lock;

void FrameProfiler::BeginFrame() {
    frameStart = std::chrono::high_resolution_clock::now();
    sectionTimes.clear();
    sectionOrder.clear();
}

void FrameProfiler::BeginSection(const std::string& name) {
    std::lock_guard<std::mutex> guard(lock);
    sectionTimes[name + "_start"] = 
        std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - frameStart).count();
}

void FrameProfiler::EndSection(const std::string& name) {
    std::lock_guard<std::mutex> guard(lock);
    double endTime = 
        std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - frameStart).count();
    double startTime = sectionTimes[name + "_start"];
    sectionTimes[name] = endTime - startTime;
    sectionTimes.erase(name + "_start");
    if (std::find(sectionOrder.begin(), sectionOrder.end(), name) == sectionOrder.end()) {
        sectionOrder.push_back(name);
    }
}

double FrameProfiler::EndFrame() {
    double frameTime = 
        std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - frameStart).count();
    return frameTime;
}

std::string FrameProfiler::GetReport() {
    std::lock_guard<std::mutex> guard(lock);
    std::stringstream ss;
    double total = 0;
    for (auto& [name, time] : sectionTimes) {
        total += time;
    }
    ss << "Frame Profile:\n";
    for (auto& name : sectionOrder) {
        double time = sectionTimes[name];
        ss << "  " << name << ": " << (time * 1000.0) << "ms (" 
           << (time / total * 100.0) << "%)\n";
    }
    ss << "Total tracked: " << (total * 1000.0) << "ms\n";
    return ss.str();
}

} // namespace NeoEngine
