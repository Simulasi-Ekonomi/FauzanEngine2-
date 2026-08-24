#pragma once

#include <cstdint>

namespace NeoEngine {
enum class AndroidLifecycleState : uint8_t { Fresh, Initialized, Active, Paused, Stopped };
enum class AndroidLifecycleError : uint8_t { None, InvalidTransition, InvalidDelta };
class AndroidLifecycleGate {
public:
    bool Initialize();
    bool Resume();
    bool Pause();
    bool Tick(float deltaSeconds);
    bool Shutdown();
    [[nodiscard]] AndroidLifecycleState State() const { return state_; }
    [[nodiscard]] uint64_t TickCount() const { return tickCount_; }
    [[nodiscard]] AndroidLifecycleError LastError() const { return lastError_; }
private:
    AndroidLifecycleState state_ = AndroidLifecycleState::Fresh;
    uint64_t tickCount_ = 0;
    AndroidLifecycleError lastError_ = AndroidLifecycleError::None;
};
} // namespace NeoEngine
