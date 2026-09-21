#include <cassert>
#include <cmath>
#include <limits>
#include "AnimationClip.h"

namespace NeoEngine
{

void AnimationClip::AddKeyframe(int bone,const Keyframe& frame)
{
    constexpr size_t kMaxBones = 4096;
    constexpr size_t kMaxKeyframesPerBone = 65536;
    constexpr float kMaxDuration = 86400.0f;
    if (bone < 0 || static_cast<size_t>(bone) >= kMaxBones) return;
    if (!std::isfinite(frame.time) || frame.time < 0.0f || frame.time > kMaxDuration) return;
    if (!std::isfinite(frame.transform.m[0][0])) return;
    if (tracks.size() >= kMaxBones && static_cast<size_t>(bone) >= tracks.size()) return;
    if (static_cast<size_t>(bone) >= tracks.size()) {
        try { tracks.resize(static_cast<size_t>(bone) + 1U); } catch (...) { return; }
    }
    auto& track = tracks[static_cast<size_t>(bone)];
    if (track.size() >= kMaxKeyframesPerBone) return;
    try { track.push_back(frame); } catch (...) { return; }
    if (frame.time > duration) duration = frame.time;
}

const std::vector<Keyframe>& AnimationClip::GetFrames(int bone) const
{
    static const std::vector<Keyframe> empty;
    if (bone < 0 || static_cast<size_t>(bone) >= tracks.size()) return empty;
    return tracks[static_cast<size_t>(bone)];
}

float AnimationClip::GetDuration() const
{
    return (std::isfinite(duration) && duration >= 0.0f) ? duration : 0.0f;
}

}
