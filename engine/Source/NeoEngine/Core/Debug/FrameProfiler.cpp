#include "FrameProfiler.h"

#include <iostream>

namespace NeoEngine
{

std::chrono::high_resolution_clock::time_point FrameProfiler::frameStart;

std::unordered_map<std::string,double> FrameProfiler::sectionTimes;
std::unordered_map<std::string,
    std::chrono::high_resolution_clock::time_point> FrameProfiler::sectionStart;

std::mutex FrameProfiler::mutex;

void FrameProfiler::BeginFrame()
{
    frameStart = std::chrono::high_resolution_clock::now();
}

void FrameProfiler::EndFrame()
{
    auto end = std::chrono::high_resolution_clock::now();

    double frameTime =
        std::chrono::duration<double,std::milli>(end-frameStart).count();

    std::cout << "Frame Time: " << frameTime << " ms\n";

    for (auto& it : sectionTimes)
    {
        std::cout << "  " << it.first
                  << ": " << it.second
                  << " ms\n";
    }

    sectionTimes.clear();
}

void FrameProfiler::BeginSection(const std::string& name)
{
    std::lock_guard<std::mutex> lock(mutex);

    sectionStart[name] =
        std::chrono::high_resolution_clock::now();
}

void FrameProfiler::EndSection(const std::string& name)
{
    auto end =
        std::chrono::high_resolution_clock::now();

    std::lock_guard<std::mutex> lock(mutex);

    auto start = sectionStart[name];

    double time =
        std::chrono::duration<double,std::milli>(end-start).count();

    sectionTimes[name] += time;
}

}
