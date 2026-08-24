#include "EngineLoop.h"
#include <stdexcept>

namespace NeoEngine {

namespace {
[[noreturn]] void LegacyLoopUnavailable() {
    throw std::logic_error("NOT_IMPLEMENTED: legacy EngineLoop has no validated renderer lifecycle; use NeoRuntime for canonical state execution");
}
}

void EngineLoop::Init() { LegacyLoopUnavailable(); }

void EngineLoop::Tick() {
    LegacyLoopUnavailable();
}

void EngineLoop::Shutdown() { LegacyLoopUnavailable(); }

}
