#pragma once

#include <cstdint>
#include <vector>

namespace NeoEngine {

struct RuntimeTimerHandle { uint16_t index = UINT16_MAX; uint16_t generation = 0; friend bool operator==(const RuntimeTimerHandle&, const RuntimeTimerHandle&) = default; };
struct RuntimeTimerFire { RuntimeTimerHandle handle{}; uint32_t userTag = 0; uint32_t fireCount = 0; };
enum class RuntimeTimerError : uint8_t { None, InvalidDuration, Capacity, InvalidHandle, AlreadyCancelled, FireCapacity };

class RuntimeTimerQueue {
public:
    static constexpr uint16_t kMaxTimers = 128, kMaxFiresPerAdvance = 256;
    bool Schedule(float intervalSeconds, bool repeating, uint32_t userTag, RuntimeTimerHandle& handle);
    bool Cancel(RuntimeTimerHandle handle);
    bool Advance(float scaledDeltaSeconds, std::vector<RuntimeTimerFire>& fires);
    [[nodiscard]] uint16_t ActiveCount() const;
    [[nodiscard]] RuntimeTimerError LastError() const { return lastError_; }
private:
    struct Timer { float interval = 0.0F; float remaining = 0.0F; uint32_t userTag = 0; uint32_t fireCount = 0; uint16_t generation = 1; bool repeating = false; bool active = false; };
    bool Fail(RuntimeTimerError error); std::vector<Timer> timers_ = std::vector<Timer>(kMaxTimers); RuntimeTimerError lastError_ = RuntimeTimerError::None;
};

} // namespace NeoEngine
