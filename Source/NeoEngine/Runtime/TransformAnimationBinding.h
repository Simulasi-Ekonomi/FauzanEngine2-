#pragma once

#include "AnimationTimeline.h"
#include "SceneWorld.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
enum class TransformChannel : uint8_t { PositionX, PositionY, PositionZ, RotationX, RotationY, RotationZ, ScaleX, ScaleY, ScaleZ };
enum class TransformAnimationError : uint8_t { None, InvalidBinding, Capacity, MissingTrack, MissingEntity, SampleFailed, TransformFailed };
struct TransformAnimationSpec { SceneEntity entity{}; std::string trackId; TransformChannel channel = TransformChannel::PositionX; float multiplier = 1.0F; float offset = 0.0F; };
class TransformAnimationBinding {
public:
    static constexpr uint16_t kMaxBindings = 256;
    bool Add(const TransformAnimationSpec& spec);
    bool Apply(SceneWorld& world, const AnimationTimeline& timeline, float time, AnimationPlayback playback);
    [[nodiscard]] TransformAnimationError LastError() const { return lastError_; }
private:
    std::vector<TransformAnimationSpec> bindings_;
    TransformAnimationError lastError_ = TransformAnimationError::None;
};
} // namespace NeoEngine
