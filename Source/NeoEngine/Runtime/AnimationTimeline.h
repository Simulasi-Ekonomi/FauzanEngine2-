#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
enum class AnimationError : uint8_t { None, InvalidTrack, DuplicateTrack, Capacity, InvalidKeys, MissingTrack };
enum class AnimationPlayback : uint8_t { Clamp, Loop };
struct AnimationKeyframe { float time = 0.0F; float value = 0.0F; };
class AnimationTimeline {
public:
    static constexpr uint8_t kMaxTracks = 64;
    static constexpr uint8_t kMaxKeysPerTrack = 128;
    bool AddTrack(std::string id, std::vector<AnimationKeyframe> keys);
    bool Sample(const std::string& id, float time, AnimationPlayback playback, float& value) const;
    [[nodiscard]] AnimationError LastError() const { return lastError_; }
private:
    struct Track { std::string id; std::vector<AnimationKeyframe> keys; };
    std::vector<Track> tracks_;
    mutable AnimationError lastError_ = AnimationError::None;
};
} // namespace NeoEngine
