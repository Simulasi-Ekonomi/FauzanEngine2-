#include "Runtime/NeoRuntime.h"

#include <cstdio>

struct Listener final : NeoEngine::RuntimeEventListener { uint32_t timers = 0; uint32_t pauses = 0; void OnRuntimeEvent(const NeoEngine::RuntimeEvent& event) override { timers += event.kind == NeoEngine::RuntimeEventKind::TimerFired ? 1U : 0U; pauses += event.kind == NeoEngine::RuntimeEventKind::RuntimePaused ? 1U : 0U; } };

int main() {
    using namespace NeoEngine; NeoRuntime runtime; Listener listener; RuntimeTimerHandle timer{};
    if (!runtime.Initialize({}) || runtime.Clock() == nullptr || runtime.Timers() == nullptr || runtime.Events() == nullptr || !runtime.Events()->Subscribe(listener) || !runtime.Timers()->Schedule(1.0F / 60.0F, false, 77, timer) || !runtime.Tick() || listener.timers != 1 || !runtime.SetPaused(true) || !runtime.Tick() || listener.pauses != 1 || runtime.Clock()->Snapshot().scaledDeltaSeconds != 0.0F || !runtime.SetPaused(false) || !runtime.Shutdown()) return 1;
    std::printf("RUNTIME_FOUNDATIONS_INTEGRATION_SMOKE_OK clock=1 timer=1 event=1 pause=1 lifecycle=1\n"); return 0;
}
