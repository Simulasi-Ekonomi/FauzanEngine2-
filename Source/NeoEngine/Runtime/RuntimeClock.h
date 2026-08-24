#pragma once

#include <cstdint>

namespace NeoEngine {

enum class RuntimeClockError : uint8_t { None, InvalidConfiguration, InvalidDelta, InvalidScale, NotInitialized };
struct RuntimeClockConfig { float fixedStepSeconds = 1.0F / 60.0F; float maxFrameDeltaSeconds = 0.25F; uint8_t maxFixedStepsPerFrame = 8; };
struct RuntimeClockSnapshot { float unscaledDeltaSeconds = 0.0F; float scaledDeltaSeconds = 0.0F; double unscaledTimeSeconds = 0.0; double scaledTimeSeconds = 0.0; uint64_t frameCount = 0; uint64_t fixedStepCount = 0; uint8_t pendingFixedSteps = 0; bool paused = false; float timeScale = 1.0F; };

class RuntimeClock {
public:
    bool Initialize(const RuntimeClockConfig& config = {});
    bool Advance(float realDeltaSeconds);
    bool SetPaused(bool paused);
    bool SetTimeScale(float scale);
    bool ConsumeFixedStep();
    [[nodiscard]] RuntimeClockSnapshot Snapshot() const { return snapshot_; }
    [[nodiscard]] RuntimeClockError LastError() const { return lastError_; }
private:
    bool Fail(RuntimeClockError error);
    RuntimeClockConfig config_{}; RuntimeClockSnapshot snapshot_{}; float accumulator_ = 0.0F; bool initialized_ = false; RuntimeClockError lastError_ = RuntimeClockError::None;
};

} // namespace NeoEngine
