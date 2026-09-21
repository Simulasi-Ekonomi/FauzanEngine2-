#include "AnimationClip.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <new>

namespace NeoEngine {

namespace {
constexpr size_t kMaxBones = 64U;
constexpr size_t kMaxKeyframesPerBone = 256U;
constexpr size_t kMaxTotalKeyframes = 4096U;
constexpr float kMaxAnimationTime = 86400.0F;

bool FiniteMatrix(const Mat4& m) {
    for (float v : m.m) if (!std::isfinite(v)) return false;
    return true;
}

Mat4 LerpMatrix(const Mat4& a, const Mat4& b, float t) {
    Mat4 out{};
    for (size_t i = 0; i < 16U; ++i) out.m[i] = a.m[i] + (b.m[i] - a.m[i]) * t;
    return out;
}
}

void AnimationClip::AddKeyframe(int bone, const Keyframe& frame) {
    if (bone < 0 || static_cast<size_t>(bone) >= kMaxBones ||
        !std::isfinite(frame.time) || frame.time < 0.0F || frame.time > kMaxAnimationTime || !FiniteMatrix(frame.transform)) return;
    if (tracks.size() <= static_cast<size_t>(bone)) {
        try {
            tracks.resize(static_cast<size_t>(bone) + 1U);
        } catch (const std::bad_alloc&) {
            return;
        }
    }
    auto& track = tracks[static_cast<size_t>(bone)];
    if (track.size() >= kMaxKeyframesPerBone) return;
    size_t total = 0U;
    for (const auto& t : tracks) {
        if (t.size() > kMaxTotalKeyframes || total > kMaxTotalKeyframes - t.size()) return;
        total += t.size();
    }
    if (total >= kMaxTotalKeyframes) return;

    auto it = std::lower_bound(track.begin(), track.end(), frame.time,
                               [](const Keyframe& value, float time) { return value.time < time; });
    bool replaced = false;
    if (it != track.end() && std::fabs(it->time - frame.time) <= 1.0e-6F) {
        *it = frame;
        replaced = true;
    } else {
        try {
            track.insert(it, frame);
        } catch (const std::bad_alloc&) {
            return;
        }
    }
    if (!replaced || frame.time >= duration) {
        duration = std::max(duration, frame.time);
    } else if (it != track.end() && it->time >= duration) {
        duration = frame.time;
    }
    if (replaced && std::isfinite(duration)) {
        float recomputed = 0.0F;
        for (const auto& candidateTrack : tracks) if (!candidateTrack.empty()) recomputed = std::max(recomputed, candidateTrack.back().time);
        duration = recomputed;
    }
}

const std::vector<Keyframe>& AnimationClip::GetFrames(int bone) const {
    static const std::vector<Keyframe> empty;
    if (bone < 0 || static_cast<size_t>(bone) >= tracks.size()) return empty;
    return tracks[static_cast<size_t>(bone)];
}

float AnimationClip::GetDuration() const { return std::isfinite(duration) && duration >= 0.0F && duration <= kMaxAnimationTime ? duration : 0.0F; }

bool AnimationClip::IsValid() const noexcept {
    if (!std::isfinite(duration) || duration < 0.0F || duration > kMaxAnimationTime) return false;
    if (tracks.size() > kMaxBones) return false;
    size_t total = 0U;
    for (const auto& track : tracks) {
        if (track.size() > kMaxKeyframesPerBone) return false;
        if (track.size() > kMaxTotalKeyframes || total > kMaxTotalKeyframes - track.size()) return false;
        total += track.size();
        float previous = -1.0F;
        for (const Keyframe& frame : track) {
            if (!std::isfinite(frame.time) || frame.time < 0.0F || frame.time > kMaxAnimationTime || !FiniteMatrix(frame.transform)) return false;
            if (frame.time <= previous) return false;
            if (frame.time > duration) return false;
            previous = frame.time;
        }
    }
    return total <= kMaxTotalKeyframes;
}

bool AnimationClip::Sample(int bone, float time, Mat4& out) const {
    const auto& track = GetFrames(bone);
    if (track.empty() || !std::isfinite(time) || !std::isfinite(duration) || time < 0.0F || time > duration) return false;
    float previous = -1.0F;
    for (const Keyframe& frame : track) {
        if (!std::isfinite(frame.time) || frame.time < 0.0F || frame.time > duration || !FiniteMatrix(frame.transform) || frame.time <= previous) return false;
        previous = frame.time;
    }
    if (track.size() == 1U || time <= track.front().time) { out = track.front().transform; return true; }
    if (time >= track.back().time) { out = track.back().transform; return true; }

    auto upper = std::upper_bound(track.begin(), track.end(), time,
                                  [](float value, const Keyframe& frame) { return value < frame.time; });
    if (upper == track.end() || upper == track.begin()) return false;
    const auto& b = *upper;
    const auto& a = *(upper - 1);
    const float span = b.time - a.time;
    if (!(span > 0.0F) || !std::isfinite(span)) return false;
    const float alpha = std::clamp((time - a.time) / span, 0.0F, 1.0F);
    if (!std::isfinite(alpha)) return false;
    Mat4 candidate = LerpMatrix(a.transform, b.transform, alpha);
    if (!FiniteMatrix(candidate)) return false;
    out = candidate;
    return true;
}

} // namespace NeoEngine
