#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
class FlipbookPlayback;
enum class AnimationPlaybackControlBindingError : uint8_t { None, InvalidConfiguration, NotConfigured, UnmappedState };
struct AnimationPlaybackControlBindingConfig { std::string pauseStateId; std::string resumeStateId; };
class AnimationPlaybackControlBinding {
public:
    bool Configure(AnimationPlaybackControlBindingConfig config);
    bool Apply(const std::string& activeStateId, FlipbookPlayback& playback);
    [[nodiscard]] AnimationPlaybackControlBindingError LastError() const { return lastError_; }
private:
    AnimationPlaybackControlBindingConfig config_{};
    bool configured_ = false;
    AnimationPlaybackControlBindingError lastError_ = AnimationPlaybackControlBindingError::NotConfigured;
};
} // namespace NeoEngine
