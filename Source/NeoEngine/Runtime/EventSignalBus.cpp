#include "EventSignalBus.h"

#include <algorithm>

namespace NeoEngine {
bool EventSignalBus::Fail(EventSignalError error) { lastError_ = error; return false; }
bool EventSignalBus::Subscribe(RuntimeEventListener& listener) { if (std::find(listeners_.begin(), listeners_.end(), &listener) != listeners_.end()) return Fail(EventSignalError::DuplicateListener); if (listeners_.size() >= kMaxListeners) return Fail(EventSignalError::Capacity); listeners_.push_back(&listener); lastError_ = EventSignalError::None; return true; }
bool EventSignalBus::Unsubscribe(RuntimeEventListener& listener) { const auto it = std::find(listeners_.begin(), listeners_.end(), &listener); if (it == listeners_.end()) return Fail(EventSignalError::MissingListener); listeners_.erase(it); lastError_ = EventSignalError::None; return true; }
bool EventSignalBus::Queue(RuntimeEvent event) { if (pending_.size() >= kMaxEvents) return Fail(EventSignalError::QueueFull); pending_.push_back(event); lastError_ = EventSignalError::None; return true; }
bool EventSignalBus::Dispatch() { for (const RuntimeEvent& event : pending_) for (RuntimeEventListener* listener : listeners_) listener->OnRuntimeEvent(event); pending_.clear(); lastError_ = EventSignalError::None; return true; }
} // namespace NeoEngine
