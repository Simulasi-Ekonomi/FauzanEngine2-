#include "Runtime/AnimationPlaybackControlBinding.h"
#include "Runtime/FlipbookPlayback.h"

namespace NeoEngine {
bool AnimationPlaybackControlBinding::Configure(AnimationPlaybackControlBindingConfig config) {
    if (config.pauseStateId.empty() || config.resumeStateId.empty() || config.pauseStateId == config.resumeStateId || config.pauseStateId.size() > 64U || config.resumeStateId.size() > 64U) { lastError_ = AnimationPlaybackControlBindingError::InvalidConfiguration; return false; }
    config_ = std::move(config); configured_ = true; lastError_ = AnimationPlaybackControlBindingError::None; return true;
}
bool AnimationPlaybackControlBinding::Apply(const std::string& activeStateId, FlipbookPlayback& playback) {
    if (!configured_) { lastError_ = AnimationPlaybackControlBindingError::NotConfigured; return false; }
    if (activeStateId == config_.pauseStateId) { playback.SetPaused(true); lastError_ = AnimationPlaybackControlBindingError::None; return true; }
    if (activeStateId == config_.resumeStateId) { playback.SetPaused(false); lastError_ = AnimationPlaybackControlBindingError::None; return true; }
    lastError_ = AnimationPlaybackControlBindingError::UnmappedState; return false;
}
} // namespace NeoEngine
