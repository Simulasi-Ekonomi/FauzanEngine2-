#pragma once
#include <cstdint>
#include <chrono>
namespace NeoEngine {
class Stats {
    static uint64_t s_FrameCount; static float s_FPS,s_DeltaTime,s_TotalTime;
    static std::chrono::high_resolution_clock::time_point s_StartTime,s_LastFrame;
public:
    static void Init(); static void FrameTick();
    static uint64_t GetFrameCount(){return s_FrameCount;}
    static float GetFPS(){return s_FPS;}
    static float GetDeltaTime(){return s_DeltaTime;}
    static float GetTotalTime(){return s_TotalTime;}
};
}
