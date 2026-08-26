#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
enum class AnimationError : uint8_t { None, InvalidTrack, DuplicateTrack, Capacity, InvalidKeys, MissingTrack, InvalidEvent, DuplicateEvent, InvalidPlayback };
enum class AnimationPlayback : uint8_t { Clamp, Loop };
struct AnimationKeyframe { float time = 0.0F; float value = 0.0F; };
struct AnimationEventMarker { std::string id; float time = 0.0F; };
class AnimationTimeline {
public:
    static constexpr uint8_t kMaxTracks = 64;
    static constexpr uint8_t kMaxKeysPerTrack = 128;
    static constexpr uint8_t kMaxEventsPerTrack = 64;
    static constexpr uint8_t kMaxIdentifierBytes = 64;
    static constexpr float kMaxEventWindowSeconds = 10.0F;
    bool AddTrack(std::string id, std::vector<AnimationKeyframe> keys);
    bool AddEventMarker(std::string trackId, AnimationEventMarker marker);
    bool CollectEvents(const std::string& trackId, float fromTime, float toTime, AnimationPlayback playback, std::vector<std::string>& output) const;
    bool Sample(const std::string& id, float time, AnimationPlayback playback, float& value) const;
    [[nodiscard]] AnimationError LastError() const { return lastError_; }
private:
    struct Track { std::string id; std::vector<AnimationKeyframe> keys; std::vector<AnimationEventMarker> events; };
    std::vector<Track> tracks_;
    mutable AnimationError lastError_ = AnimationError::None;
};
} // namespace NeoEngine
