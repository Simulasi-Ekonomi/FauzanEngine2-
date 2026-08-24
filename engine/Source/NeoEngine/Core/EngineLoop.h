#pragma once

namespace NeoEngine {

class EngineLoop {
public:
    static void Init();
    static void Run();
    static void Shutdown();
    static bool IsRunning();
    static float GetDeltaTime();
    static int GetFrameCount();
};

} // namespace NeoEngine
