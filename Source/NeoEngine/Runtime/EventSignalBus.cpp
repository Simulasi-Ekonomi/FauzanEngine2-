#include "EventSignalBus.h"

#include <algorithm>

namespace NeoEngine {
bool EventSignalBus::Fail(EventSignalError error) { lastError_ = error; return false; }
bool EventSignalBus::Subscribe(RuntimeEventListener& listener) { if (std::find(listeners_.begin(), listeners_.end(), &listener) != listeners_.end()) return Fail(EventSignalError::DuplicateListener); if (listeners_.size() >= kMaxListeners) return Fail(EventSignalError::Capacity); listeners_.push_back(&listener); lastError_ = EventSignalError::None; return true; }
bool EventSignalBus::Unsubscribe(RuntimeEventListener& listener) { const auto it = std::find(listeners_.begin(), listeners_.end(), &listener); if (it == listeners_.end()) return Fail(EventSignalError::MissingListener); listeners_.erase(it); lastError_ = EventSignalError::None; return true; }
bool EventSignalBus::Queue(RuntimeEvent event) { if (pending_.size() >= kMaxEvents) return Fail(EventSignalError::QueueFull); pending_.push_back(event); lastError_ = EventSignalError::None; return true; }
bool EventSignalBus::Dispatch(EventSignalDispatchReceipt* receipt) { uint64_t digest = 1469598103934665603ULL; const auto mix = [&digest](uint64_t value) { for (uint8_t index = 0U; index < 8U; ++index) { digest ^= static_cast<uint8_t>(value >> (index * 8U)); digest *= 1099511628211ULL; } }; for (const RuntimeEvent& event : pending_) { mix(static_cast<uint8_t>(event.kind)); mix(event.subjectId); mix(static_cast<uint32_t>(event.value)); mix(event.tick); } for (const RuntimeEvent& event : pending_) for (RuntimeEventListener* listener : listeners_) listener->OnRuntimeEvent(event); const EventSignalDispatchReceipt candidate{static_cast<uint16_t>(listeners_.size()), static_cast<uint16_t>(pending_.size()), digest}; pending_.clear(); lastError_ = EventSignalError::None; if (receipt != nullptr) *receipt = candidate; return true; }
} // namespace NeoEngine
