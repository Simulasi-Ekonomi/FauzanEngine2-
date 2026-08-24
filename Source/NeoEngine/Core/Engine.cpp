#include "Engine.h"
#include "EngineLoop.h"

namespace NeoEngine {

Engine& Engine::Get() {
    static Engine instance;
    return instance;
}

void Engine::Start() {
    initialized = true;
    EngineLoop::Init();

    while (running) {
        EngineLoop::Tick();
    }

    EngineLoop::Shutdown();
}

bool Engine::IsInitialized() const {
    return initialized;
}

bool Engine::IsRunning() const {
    return running;
}

void Engine::Stop() {
    running = false;
}

} // namespace NeoEngine
