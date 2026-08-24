#pragma once

namespace NeoEngine {

class EngineLoop {
public:
    static void Init();
    static void Tick();
    static void Shutdown();
};

} // namespace NeoEngine
