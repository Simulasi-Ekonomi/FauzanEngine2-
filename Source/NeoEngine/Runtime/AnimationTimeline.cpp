#include "AnimationTimeline.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {
bool AnimationTimeline::AddTrack(std::string id, std::vector<AnimationKeyframe> keys) { if(id.empty()||id.size()>64U){lastError_=AnimationError::InvalidTrack;return false;} if(std::any_of(tracks_.begin(),tracks_.end(),[&id](const Track& track){return track.id==id;})){lastError_=AnimationError::DuplicateTrack;return false;} if(tracks_.size()>=kMaxTracks||keys.size()>kMaxKeysPerTrack){lastError_=AnimationError::Capacity;return false;} if(keys.size()<2U||!std::isfinite(keys.front().time)||keys.front().time!=0.0F){lastError_=AnimationError::InvalidKeys;return false;} for(size_t index=0;index<keys.size();++index){if(!std::isfinite(keys[index].time)||!std::isfinite(keys[index].value)||(index>0U&&keys[index].time<=keys[index-1U].time)){lastError_=AnimationError::InvalidKeys;return false;}} tracks_.push_back({std::move(id),std::move(keys)});lastError_=AnimationError::None;return true; }
bool AnimationTimeline::AddEventMarker(std::string trackId, AnimationEventMarker marker) {
    const auto found = std::find_if(tracks_.begin(), tracks_.end(), [&trackId](const Track& track) { return track.id == trackId; });
    if (found == tracks_.end()) { lastError_ = AnimationError::MissingTrack; return false; }
    if (marker.id.empty() || marker.id.size() > kMaxIdentifierBytes || !std::isfinite(marker.time) || marker.time < 0.0F || marker.time > found->keys.back().time) { lastError_ = AnimationError::InvalidEvent; return false; }
    if (std::any_of(found->events.begin(), found->events.end(), [&marker](const AnimationEventMarker& event) { return event.id == marker.id; })) { lastError_ = AnimationError::DuplicateEvent; return false; }
    if (found->events.size() >= kMaxEventsPerTrack) { lastError_ = AnimationError::Capacity; return false; }
    found->events.push_back(std::move(marker));
    std::sort(found->events.begin(), found->events.end(), [](const AnimationEventMarker& left, const AnimationEventMarker& right) { return left.time == right.time ? left.id < right.id : left.time < right.time; });
    lastError_ = AnimationError::None;
    return true;
}

bool AnimationTimeline::CollectEvents(const std::string& trackId, float fromTime, float toTime, AnimationPlayback playback, std::vector<std::string>& output) const {
    const auto found = std::find_if(tracks_.begin(), tracks_.end(), [&trackId](const Track& track) { return track.id == trackId; });
    if (found == tracks_.end()) { lastError_ = AnimationError::MissingTrack; return false; }
    const float duration = found->keys.back().time;
    if (!std::isfinite(fromTime) || !std::isfinite(toTime) || fromTime > toTime || toTime - fromTime > kMaxEventWindowSeconds) { lastError_ = AnimationError::InvalidEvent; return false; }
    std::vector<std::string> candidate;
    const auto appendRange = [&found, &candidate](float start, float end) {
        for (const AnimationEventMarker& event : found->events) if (event.time > start && event.time <= end) candidate.push_back(event.id);
    };
    if (playback == AnimationPlayback::Loop) {
        if (toTime - fromTime >= duration) { lastError_ = AnimationError::InvalidEvent; return false; }
        const auto normalize = [duration](float time) { float normalized = std::fmod(time, duration); if (normalized < 0.0F) normalized += duration; return normalized; };
        const float start = normalize(fromTime);
        const float end = normalize(toTime);
        if (end >= start) appendRange(start, end);
        else { appendRange(start, duration); appendRange(-0.000001F, end); }
    } else {
        const float start = std::clamp(fromTime, 0.0F, duration);
        const float end = std::clamp(toTime, 0.0F, duration);
        if (end >= start) appendRange(start, end);
    }
    output = std::move(candidate);
    lastError_ = AnimationError::None;
    return true;
}

bool AnimationTimeline::Sample(const std::string& id,float time,AnimationPlayback playback,float& value) const { const auto found=std::find_if(tracks_.begin(),tracks_.end(),[&id](const Track& track){return track.id==id;}); if(found==tracks_.end()){lastError_=AnimationError::MissingTrack;return false;} if(!std::isfinite(time)){lastError_=AnimationError::InvalidKeys;return false;} const std::vector<AnimationKeyframe>& keys=found->keys;const float duration=keys.back().time;float sampleTime=time;if(playback==AnimationPlayback::Loop){sampleTime=std::fmod(time,duration);if(sampleTime<0.0F)sampleTime+=duration;}else sampleTime=std::clamp(time,0.0F,duration); if(sampleTime>=duration){value=keys.back().value;lastError_=AnimationError::None;return true;} const auto upper=std::upper_bound(keys.begin(),keys.end(),sampleTime,[](float needle,const AnimationKeyframe& key){return needle<key.time;});const size_t right=static_cast<size_t>(upper-keys.begin()),left=right-1U;const float span=keys[right].time-keys[left].time,alpha=(sampleTime-keys[left].time)/span;value=keys[left].value+(keys[right].value-keys[left].value)*alpha;lastError_=AnimationError::None;return true; }
} // namespace NeoEngine
