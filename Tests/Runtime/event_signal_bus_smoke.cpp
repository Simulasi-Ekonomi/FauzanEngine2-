#include "Runtime/EventSignalBus.h"
#include <cassert>
#include <cstdint>

namespace {
class Listener final : public NeoEngine::RuntimeEventListener {
public:
    void OnRuntimeEvent(const NeoEngine::RuntimeEvent& event) override { ++count; last = event; }
    uint32_t count = 0U;
    NeoEngine::RuntimeEvent last{};
};
}

int main() {
    using namespace NeoEngine;
    EventSignalBus bus;
    Listener listener;
    assert(bus.Subscribe(listener));
    assert(!bus.Subscribe(listener));
    assert(bus.LastError() == EventSignalError::DuplicateListener);
    const RuntimeEvent event{RuntimeEventKind::WorldMutation, 42U, -7, 99U};
    assert(bus.Queue(event));
    EventSignalDispatchReceipt receipt{};
    assert(bus.Dispatch(&receipt));
    assert(listener.count == 1U);
    assert(listener.last.kind == event.kind && listener.last.subjectId == event.subjectId && listener.last.value == event.value && listener.last.tick == event.tick);
    assert(receipt.listenerCount == 1U && receipt.eventCount == 1U);
    assert(bus.PendingCount() == 0U);
    assert(bus.Unsubscribe(listener));
    assert(!bus.Unsubscribe(listener));
    assert(bus.LastError() == EventSignalError::MissingListener);
    Listener listeners[EventSignalBus::kMaxListeners];
    for (uint16_t i = 0U; i < EventSignalBus::kMaxListeners; ++i) assert(bus.Subscribe(listeners[i]));
    Listener overflowListener;
    assert(!bus.Subscribe(overflowListener));
    assert(bus.LastError() == EventSignalError::Capacity);
    assert(bus.ListenerCount() == EventSignalBus::kMaxListeners);
    for (uint16_t i = 0U; i < EventSignalBus::kMaxEvents; ++i) assert(bus.Queue({RuntimeEventKind::TimerFired, i, static_cast<int32_t>(i), i}));
    assert(!bus.Queue({RuntimeEventKind::TimerFired, 999U, 0, 999U}));
    assert(bus.LastError() == EventSignalError::QueueFull);
    assert(bus.PendingCount() == EventSignalBus::kMaxEvents);
    assert(bus.Dispatch());
    assert(bus.PendingCount() == 0U);
    return 0;
}
