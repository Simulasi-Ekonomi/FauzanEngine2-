#include "Stats.h"

namespace NeoEngine {

uint64_t Stats::s_FrameCount = 0;
float Stats::s_FPS = 0.0f;
float Stats::s_DeltaTime = 0.0f;
float Stats::s_TotalTime = 0.0f;
std::chrono::high_resolution_clock::time_point Stats::s_StartTime;
std::chrono::high_resolution_clock::time_point Stats::s_LastFrame;

void Stats::Init() {
    s_StartTime = std::chrono::high_resolution_clock::now();
    s_LastFrame = s_StartTime;
    s_FrameCount = 0;
    s_TotalTime = 0.0f;
}

void Stats::FrameTick() {
    auto now = std::chrono::high_resolution_clock::now();
    s_DeltaTime = std::chrono::duration<float>(now - s_LastFrame).count();
    s_LastFrame = now;
    s_FrameCount++;
    s_TotalTime += s_DeltaTime;
    
    // Update FPS setiap detik
    static float fpsTimer = 0.0f;
    static uint64_t fpsFrames = 0;
    fpsTimer += s_DeltaTime;
    fpsFrames++;
    if (fpsTimer >= 1.0f) {
        s_FPS = static_cast<float>(fpsFrames) / fpsTimer;
        fpsTimer = 0.0f;
        fpsFrames = 0;
    }
}

uint64_t Stats::GetFrameCount() { return s_FrameCount; }
float Stats::GetFPS() { return s_FPS; }
float Stats::GetDeltaTime() { return s_DeltaTime; }
float Stats::GetTotalTime() { return s_TotalTime; }

} // namespace NeoEngine
