#pragma once

#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class RuntimeEventKind : uint8_t { RuntimePaused, RuntimeResumed, TimerFired, InputAction, WorldMutation, AuthoringMutation };
struct RuntimeEvent { RuntimeEventKind kind = RuntimeEventKind::RuntimePaused; uint32_t subjectId = 0; int32_t value = 0; uint64_t tick = 0; };
enum class EventSignalError : uint8_t { None, DuplicateListener, MissingListener, Capacity, QueueFull };

class RuntimeEventListener { public: virtual ~RuntimeEventListener() = default; virtual void OnRuntimeEvent(const RuntimeEvent& event) = 0; };

class EventSignalBus {
public:
    static constexpr uint16_t kMaxListeners = 64, kMaxEvents = 512;
    bool Subscribe(RuntimeEventListener& listener);
    bool Unsubscribe(RuntimeEventListener& listener);
    bool Queue(RuntimeEvent event);
    bool Dispatch();
    [[nodiscard]] uint16_t ListenerCount() const { return static_cast<uint16_t>(listeners_.size()); }
    [[nodiscard]] uint16_t PendingCount() const { return static_cast<uint16_t>(pending_.size()); }
    [[nodiscard]] EventSignalError LastError() const { return lastError_; }
private:
    bool Fail(EventSignalError error);
    std::vector<RuntimeEventListener*> listeners_; std::vector<RuntimeEvent> pending_; EventSignalError lastError_ = EventSignalError::None;
};

} // namespace NeoEngine
