#include "Runtime/EventSignalBus.h"

#include <cstdio>

struct Listener final : NeoEngine::RuntimeEventListener { uint32_t count = 0; uint64_t tickSum = 0; void OnRuntimeEvent(const NeoEngine::RuntimeEvent& event) override { ++count; tickSum += event.tick; } };

int main() {
    using namespace NeoEngine;
    EventSignalBus bus;
    Listener first{}, second{};
    EventSignalDispatchReceipt initialReceipt{};
    if (!bus.Subscribe(first) || !bus.Subscribe(second) || bus.Subscribe(first) || bus.LastError() != EventSignalError::DuplicateListener || !bus.Queue({RuntimeEventKind::InputAction, 7, 1, 3}) || !bus.Queue({RuntimeEventKind::WorldMutation, 9, 2, 5}) || bus.PendingCount() != 2 || !bus.Dispatch(&initialReceipt) || initialReceipt.listenerCount != 2U || initialReceipt.eventCount != 2U || initialReceipt.eventDigest == 0U || first.count != 2 || second.count != 2 || first.tickSum != 8 || !bus.Unsubscribe(second) || !bus.Queue({RuntimeEventKind::RuntimePaused, 0, 0, 9})) return 1;
    EventSignalDispatchReceipt finalReceipt{};
    if (!bus.Dispatch(&finalReceipt) || finalReceipt.listenerCount != 1U || finalReceipt.eventCount != 1U || finalReceipt.eventDigest == 0U || finalReceipt.eventDigest == initialReceipt.eventDigest || first.count != 3 || second.count != 2 || bus.Unsubscribe(second) || bus.LastError() != EventSignalError::MissingListener) return 1;
    std::printf("EVENT_SIGNAL_BUS_SMOKE_OK listeners=2 ordered=1 dispatchReceipt=1 unsubscribe=1 bounded=1\n");
    return 0;
}
