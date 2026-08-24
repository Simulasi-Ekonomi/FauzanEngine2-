#pragma once
#include <cstdint>
#include <chrono>

namespace NeoEngine {

class Stats {
public:
    static void Init();
    static void FrameTick();
    static uint64_t GetFrameCount();
    static float GetFPS();
    static float GetDeltaTime();
    static float GetTotalTime();

private:
    static uint64_t s_FrameCount;
    static float s_FPS;
    static float s_DeltaTime;
    static float s_TotalTime;
    static std::chrono::high_resolution_clock::time_point s_StartTime;
    static std::chrono::high_resolution_clock::time_point s_LastFrame;
};

} // namespace NeoEngine
