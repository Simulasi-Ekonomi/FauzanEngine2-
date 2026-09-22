#include "RuntimeTimerQueue.h"

#include <cmath>

namespace NeoEngine {
bool RuntimeTimerQueue::Fail(RuntimeTimerError error) { lastError_ = error; return false; }
bool RuntimeTimerQueue::Schedule(float interval, bool repeating, uint32_t tag, RuntimeTimerHandle& handle) { if (!(interval > 0.0F) || !std::isfinite(interval)) return Fail(RuntimeTimerError::InvalidDuration); for (uint16_t index = 0; index < kMaxTimers; ++index) { Timer& timer = timers_[index]; if (!timer.active) { timer.interval = interval; timer.remaining = interval; timer.userTag = tag; timer.fireCount = 0; timer.repeating = repeating; timer.active = true; handle = {index, timer.generation}; lastError_ = RuntimeTimerError::None; return true; } } return Fail(RuntimeTimerError::Capacity); }
bool RuntimeTimerQueue::Cancel(RuntimeTimerHandle handle) { if (handle.index >= kMaxTimers) return Fail(RuntimeTimerError::InvalidHandle); Timer& timer = timers_[handle.index]; if (timer.generation != handle.generation) return Fail(RuntimeTimerError::InvalidHandle); if (!timer.active) return Fail(RuntimeTimerError::AlreadyCancelled); timer.active = false; ++timer.generation; if (timer.generation == 0) ++timer.generation; lastError_ = RuntimeTimerError::None; return true; }
bool RuntimeTimerQueue::Advance(float delta, std::vector<RuntimeTimerFire>& fires) {
    if (!(delta >= 0.0F) || !std::isfinite(delta)) return Fail(RuntimeTimerError::InvalidDuration);

    // Preflight the complete advance before mutating timer state. This keeps a
    // FireCapacity failure transactional: callers can retry without losing
    // elapsed time or fire-count state.
    uint32_t fireCount = 0;
    for (const Timer& timer : timers_) {
        if (!timer.active) continue;
        const float nextRemaining = timer.remaining - delta;
        if (!timer.repeating) {
            if (nextRemaining <= 0.000001F) {
                ++fireCount;
                if (fireCount > kMaxFiresPerAdvance) return Fail(RuntimeTimerError::FireCapacity);
            }
            continue;
        }

        float remaining = nextRemaining;
        uint32_t timerFires = 0;
        while (remaining <= 0.000001F) {
            ++timerFires;
            if (timerFires > kMaxFiresPerAdvance || fireCount + timerFires > kMaxFiresPerAdvance) return Fail(RuntimeTimerError::FireCapacity);
            remaining += timer.interval;
        }
        fireCount += timerFires;
    }

    try {
        fires.reserve(fireCount);
    } catch (...) {
        return Fail(RuntimeTimerError::FireCapacity);
    }

    fires.clear();
    for (uint16_t index = 0; index < kMaxTimers; ++index) {
        Timer& timer = timers_[index];
        if (!timer.active) continue;
        timer.remaining -= delta;
        while (timer.active && timer.remaining <= 0.000001F) {
            ++timer.fireCount;
            fires.push_back({{index, timer.generation}, timer.userTag, timer.fireCount});
            if (!timer.repeating) {
                timer.active = false;
                ++timer.generation;
                if (timer.generation == 0) ++timer.generation;
                break;
            }
            timer.remaining += timer.interval;
        }
    }
    lastError_ = RuntimeTimerError::None;
    return true;
}
uint16_t RuntimeTimerQueue::ActiveCount() const { uint16_t count = 0; for (const Timer& timer : timers_) count += timer.active ? 1U : 0U; return count; }
} // namespace NeoEngine
