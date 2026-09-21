#include "RuntimeClock.h"

#include <cmath>
#include <limits>

namespace NeoEngine {
bool RuntimeClock::Fail(RuntimeClockError error) { lastError_ = error; return false; }
bool RuntimeClock::Initialize(const RuntimeClockConfig& config) {
    if (!(config.fixedStepSeconds > 0.0F) || !(config.maxFrameDeltaSeconds >= config.fixedStepSeconds) || config.maxFixedStepsPerFrame == 0 || !std::isfinite(config.fixedStepSeconds) || !std::isfinite(config.maxFrameDeltaSeconds)) return Fail(RuntimeClockError::InvalidConfiguration);
    config_ = config; snapshot_ = {}; snapshot_.timeScale = 1.0F; accumulator_ = 0.0F; initialized_ = true; lastError_ = RuntimeClockError::None; return true;
}
bool RuntimeClock::SetPaused(bool paused) { if (!initialized_) return Fail(RuntimeClockError::NotInitialized); snapshot_.paused = paused; lastError_ = RuntimeClockError::None; return true; }
bool RuntimeClock::SetTimeScale(float scale) { if (!initialized_) return Fail(RuntimeClockError::NotInitialized); if (!(scale >= 0.0F) || scale > 4.0F || !std::isfinite(scale)) return Fail(RuntimeClockError::InvalidScale); snapshot_.timeScale = scale; lastError_ = RuntimeClockError::None; return true; }
bool RuntimeClock::Advance(float realDeltaSeconds) {
    if (!initialized_) return Fail(RuntimeClockError::NotInitialized); if (!(realDeltaSeconds >= 0.0F) || !std::isfinite(realDeltaSeconds)) return Fail(RuntimeClockError::InvalidDelta);
    snapshot_.unscaledDeltaSeconds = realDeltaSeconds > config_.maxFrameDeltaSeconds ? config_.maxFrameDeltaSeconds : realDeltaSeconds;
    snapshot_.scaledDeltaSeconds = snapshot_.paused ? 0.0F : snapshot_.unscaledDeltaSeconds * snapshot_.timeScale;
    if (!std::isfinite(snapshot_.scaledDeltaSeconds)) return Fail(RuntimeClockError::Overflow);
    const float nextUnscaled = snapshot_.unscaledTimeSeconds + snapshot_.unscaledDeltaSeconds;
    const float nextScaled = snapshot_.scaledTimeSeconds + snapshot_.scaledDeltaSeconds;
    if (!std::isfinite(nextUnscaled) || !std::isfinite(nextScaled)) return Fail(RuntimeClockError::Overflow);
    if (snapshot_.frameCount == std::numeric_limits<uint64_t>::max()) return Fail(RuntimeClockError::Overflow);
    snapshot_.unscaledTimeSeconds = nextUnscaled; snapshot_.scaledTimeSeconds = nextScaled; ++snapshot_.frameCount;
    accumulator_ += snapshot_.scaledDeltaSeconds;
    if (!std::isfinite(accumulator_) || accumulator_ < 0.0F) return Fail(RuntimeClockError::Overflow);
    uint8_t steps = 0; while (accumulator_ >= config_.fixedStepSeconds && steps < config_.maxFixedStepsPerFrame) { accumulator_ -= config_.fixedStepSeconds; ++steps; }
    if (steps == config_.maxFixedStepsPerFrame && accumulator_ >= config_.fixedStepSeconds) {
        const float dropped = std::floor(accumulator_ / config_.fixedStepSeconds) * config_.fixedStepSeconds;
        if (!std::isfinite(dropped) || dropped < 0.0F) return Fail(RuntimeClockError::Overflow);
        accumulator_ -= dropped;
        snapshot_.droppedFixedSeconds += dropped;
        const float droppedSteps = dropped / config_.fixedStepSeconds;
        if (!std::isfinite(droppedSteps) || droppedSteps > static_cast<float>(std::numeric_limits<uint64_t>::max())) return Fail(RuntimeClockError::Overflow);
        snapshot_.droppedFixedStepCount += static_cast<uint64_t>(droppedSteps);
        lastError_ = RuntimeClockError::FixedStepOverrun;
        snapshot_.pendingFixedSteps = steps;
        return true;
    }
    snapshot_.pendingFixedSteps = steps; lastError_ = RuntimeClockError::None; return true;
}
bool RuntimeClock::ConsumeFixedStep() { if (!initialized_ || snapshot_.pendingFixedSteps == 0) return false; --snapshot_.pendingFixedSteps; ++snapshot_.fixedStepCount; return true; }
} // namespace NeoEngine
