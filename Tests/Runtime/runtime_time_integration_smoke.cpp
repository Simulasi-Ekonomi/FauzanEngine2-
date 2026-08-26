#include "Runtime/NeoRuntime.h"

#include <cstdint>

namespace {
struct Listener final : NeoEngine::RuntimeEventListener {
    uint32_t timeEvents = 0U;
    uint32_t dayEvents = 0U;
    uint32_t phaseEvents = 0U;
    uint32_t pausedEvents = 0U;
    void OnRuntimeEvent(const NeoEngine::RuntimeEvent& event) override {
        timeEvents += event.kind == NeoEngine::RuntimeEventKind::GameTimeChanged ? 1U : 0U;
        dayEvents += event.kind == NeoEngine::RuntimeEventKind::GameDayChanged ? 1U : 0U;
        phaseEvents += event.kind == NeoEngine::RuntimeEventKind::GamePhaseChanged ? 1U : 0U;
        pausedEvents += event.kind == NeoEngine::RuntimeEventKind::RuntimePaused ? 1U : 0U;
    }
};
}

int main() {
    using namespace NeoEngine;
    NeoRuntime runtime;
    RuntimeConfig config{};
    config.farmWidth = 4U;
    config.farmHeight = 4U;
    config.farmNpcCount = 1U;
    config.renderWidth = 96U;
    config.renderHeight = 96U;
    config.timeConfig = {60U, 1440U, 360U, 1080U, 1000U, 4000U, 64U};
    if (!runtime.Initialize(config) || runtime.Time() == nullptr || runtime.Time()->Snapshot().totalGameMinutes != 0U) return 1;
    Listener listener;
    if (runtime.Events() == nullptr || !runtime.Events()->Subscribe(listener)) return 2;

    if (!runtime.Tick() || runtime.Time()->Snapshot().totalGameMinutes != 60U || listener.timeEvents != 1U || listener.phaseEvents != 0U) return 3;
    if (!runtime.SetPaused(true) || !runtime.Tick() || runtime.Time()->Snapshot().totalGameMinutes != 60U || listener.pausedEvents != 1U) return 4;
    if (!runtime.SetPaused(false) || !runtime.SetTimeScalePermille(500U) || !runtime.Tick() || runtime.Time()->Snapshot().totalGameMinutes != 90U) return 5;
    if (!runtime.Tick() || runtime.Time()->Snapshot().totalGameMinutes != 120U) return 6;

    if (!runtime.SetTimeScalePermille(1000U)) return 7;
    for (uint32_t tick = 0U; tick < 28U; ++tick) if (!runtime.Tick()) return 8;
    if (runtime.Time()->Snapshot().dayIndex != 1U || listener.dayEvents != 1U || listener.phaseEvents != 3U) return 9;
    if (!runtime.Shutdown() || runtime.State() != RuntimeState::Shutdown) return 10;
    return 0;
}
