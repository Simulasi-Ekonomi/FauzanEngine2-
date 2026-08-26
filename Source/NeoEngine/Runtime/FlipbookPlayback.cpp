#include "Runtime/FlipbookPlayback.h"

#include <cmath>

namespace NeoEngine {
bool FlipbookPlayback::Initialize(FlipbookPlaybackConfig config) {
    if (!std::isfinite(config.durationSeconds) || config.durationSeconds <= 0.0F || config.durationSeconds > 60.0F) { initialized_ = false; lastError_ = FlipbookPlaybackError::InvalidConfiguration; return false; }
    config_ = config; timeSeconds_ = 0.0F; paused_ = false; initialized_ = true; lastError_ = FlipbookPlaybackError::None; return true;
}
bool FlipbookPlayback::Advance(float deltaSeconds, float& normalizedSample) {
    if (!initialized_) { lastError_ = FlipbookPlaybackError::NotInitialized; return false; }
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0F || deltaSeconds > 60.0F) { lastError_ = FlipbookPlaybackError::InvalidDelta; return false; }
    float candidate = paused_ ? timeSeconds_ : timeSeconds_ + deltaSeconds;
    if (config_.loop) candidate = std::fmod(candidate, config_.durationSeconds); else if (candidate > config_.durationSeconds) candidate = config_.durationSeconds;
    const float sample = candidate / config_.durationSeconds;
    if (!std::isfinite(candidate) || !std::isfinite(sample) || sample < 0.0F || sample > 1.0F) { lastError_ = FlipbookPlaybackError::InvalidDelta; return false; }
    timeSeconds_ = candidate; normalizedSample = sample; lastError_ = FlipbookPlaybackError::None; return true;
}
} // namespace NeoEngine
