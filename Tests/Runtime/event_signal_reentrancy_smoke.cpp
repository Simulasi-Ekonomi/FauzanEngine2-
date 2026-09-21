#include "Runtime/EventSignalBus.h"
#include <cstdio>
class MutatingListener final : public NeoEngine::RuntimeEventListener {
public:
    explicit MutatingListener(NeoEngine::EventSignalBus& bus) : bus_(bus) {}
    void OnRuntimeEvent(const NeoEngine::RuntimeEvent&) override {
        ++calls;
        if (calls == 1U) {
            bus_.Queue({NeoEngine::RuntimeEventKind::WorldMutation, 2U, 7, 2U});
        }
    }
    NeoEngine::EventSignalBus& bus_;
    uint32_t calls = 0U;
};
class CountingListener final : public NeoEngine::RuntimeEventListener {
public:
    void OnRuntimeEvent(const NeoEngine::RuntimeEvent&) override { ++calls; }
    uint32_t calls = 0U;
};
int main() {
    using namespace NeoEngine;
    EventSignalBus bus;
    MutatingListener mutator(bus);
    CountingListener counter;
    if (!bus.Subscribe(mutator) || !bus.Subscribe(counter)) return 1;
    if (!bus.Queue({RuntimeEventKind::WorldMutation, 1U, 3, 1U})) return 2;
    EventSignalDispatchReceipt first{};
    if (!bus.Dispatch(&first) || first.eventCount != 1U || first.listenerCount != 2U || bus.PendingCount() != 1U) return 3;
    EventSignalDispatchReceipt second{};
    if (!bus.Dispatch(&second) || second.eventCount != 1U || bus.PendingCount() != 0U) return 4;
    if (mutator.calls != 2U || counter.calls != 2U) return 5;
    std::printf("EVENT_SIGNAL_REENTRANCY_SMOKE_OK snapshot_dispatch=1 queued_during_callback=1\\n");
    return 0;
}
