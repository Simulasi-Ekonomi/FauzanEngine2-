#include "EngineLoop.h"
#include <chrono>

namespace NeoEngine {

static bool g_Running = false;
static float g_DeltaTime = 0;
static int g_FrameCount = 0;

void EngineLoop::Init() { g_Running = true; }
void EngineLoop::Run() {
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    g_DeltaTime = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;
    g_FrameCount++;
}
void EngineLoop::Shutdown() { g_Running = false; }
bool EngineLoop::IsRunning() { return g_Running; }
float EngineLoop::GetDeltaTime() { return g_DeltaTime; }
int EngineLoop::GetFrameCount() { return g_FrameCount; }

} // namespace NeoEngine
