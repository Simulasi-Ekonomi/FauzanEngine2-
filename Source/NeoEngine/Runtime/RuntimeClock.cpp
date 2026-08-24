#include "RuntimeClock.h"

#include <cmath>

namespace NeoEngine {
bool RuntimeClock::Fail(RuntimeClockError error) { lastError_ = error; return false; }
bool RuntimeClock::Initialize(const RuntimeClockConfig& config) {
    if (!(config.fixedStepSeconds > 0.0F) || !(config.maxFrameDeltaSeconds >= config.fixedStepSeconds) || config.maxFixedStepsPerFrame == 0) return Fail(RuntimeClockError::InvalidConfiguration);
    config_ = config; snapshot_ = {}; snapshot_.timeScale = 1.0F; accumulator_ = 0.0F; initialized_ = true; lastError_ = RuntimeClockError::None; return true;
}
bool RuntimeClock::SetPaused(bool paused) { if (!initialized_) return Fail(RuntimeClockError::NotInitialized); snapshot_.paused = paused; lastError_ = RuntimeClockError::None; return true; }
bool RuntimeClock::SetTimeScale(float scale) { if (!initialized_) return Fail(RuntimeClockError::NotInitialized); if (!(scale >= 0.0F) || scale > 4.0F || !std::isfinite(scale)) return Fail(RuntimeClockError::InvalidScale); snapshot_.timeScale = scale; lastError_ = RuntimeClockError::None; return true; }
bool RuntimeClock::Advance(float realDeltaSeconds) {
    if (!initialized_) return Fail(RuntimeClockError::NotInitialized); if (!(realDeltaSeconds >= 0.0F) || !std::isfinite(realDeltaSeconds)) return Fail(RuntimeClockError::InvalidDelta);
    snapshot_.unscaledDeltaSeconds = realDeltaSeconds > config_.maxFrameDeltaSeconds ? config_.maxFrameDeltaSeconds : realDeltaSeconds;
    snapshot_.scaledDeltaSeconds = snapshot_.paused ? 0.0F : snapshot_.unscaledDeltaSeconds * snapshot_.timeScale;
    snapshot_.unscaledTimeSeconds += snapshot_.unscaledDeltaSeconds; snapshot_.scaledTimeSeconds += snapshot_.scaledDeltaSeconds; ++snapshot_.frameCount;
    accumulator_ += snapshot_.scaledDeltaSeconds; uint8_t steps = 0; while (accumulator_ + 0.000001F >= config_.fixedStepSeconds && steps < config_.maxFixedStepsPerFrame) { accumulator_ -= config_.fixedStepSeconds; ++steps; }
    if (steps == config_.maxFixedStepsPerFrame && accumulator_ >= config_.fixedStepSeconds) accumulator_ = 0.0F;
    snapshot_.pendingFixedSteps = steps; lastError_ = RuntimeClockError::None; return true;
}
bool RuntimeClock::ConsumeFixedStep() { if (!initialized_ || snapshot_.pendingFixedSteps == 0) return false; --snapshot_.pendingFixedSteps; ++snapshot_.fixedStepCount; return true; }
} // namespace NeoEngine
