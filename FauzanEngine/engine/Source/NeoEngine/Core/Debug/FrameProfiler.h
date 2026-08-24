#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>

namespace NeoEngine
{

class FrameProfiler
{
public:

    static void BeginFrame();
    static void EndFrame();

    static void BeginSection(const std::string& name);
    static void EndSection(const std::string& name);

private:

    static std::chrono::high_resolution_clock::time_point frameStart;

    static std::unordered_map<std::string,double> sectionTimes;
    static std::unordered_map<std::string,
        std::chrono::high_resolution_clock::time_point> sectionStart;

    static std::mutex mutex;
};

}
