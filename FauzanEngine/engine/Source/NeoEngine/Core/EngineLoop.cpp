#include "EngineLoop.h"
#include "../Platform/Common/Platform.h"
#include <iostream>

void EngineLoop::Init() {
    Platform::Get().Init();
    std::cout << "EngineLoop Initialized" << std::endl;
}

void EngineLoop::Tick() {
    Platform::Get().PumpEvents();
}

void EngineLoop::Shutdown() {
    std::cout << "EngineLoop Shutdown" << std::endl;
    Platform::Get().Shutdown();
}
