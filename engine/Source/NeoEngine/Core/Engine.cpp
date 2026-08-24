#include "Engine.h"
#include "EngineLoop.h"

Engine& Engine::Get() {
    static Engine instance;
    return instance;
}

void Engine::Init() {
    EngineLoop::Init();
    bIsRunning = true;
}

void Engine::Run() {
    while (bIsRunning) {
        EngineLoop::Tick();
    }
}

void Engine::Shutdown() {
    EngineLoop::Shutdown();
    bIsRunning = false;
}
