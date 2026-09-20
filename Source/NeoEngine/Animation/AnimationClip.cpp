#include "AnimationClip.h"
#include <algorithm>
#include <cmath>

namespace NeoEngine {

namespace {
constexpr size_t kMaxBones = 64U;
constexpr size_t kMaxKeyframesPerBone = 256U;
constexpr size_t kMaxTotalKeyframes = 4096U;

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
        !std::isfinite(frame.time) || frame.time < 0.0F || !FiniteMatrix(frame.transform)) return;
    if (tracks.size() <= static_cast<size_t>(bone)) tracks.resize(static_cast<size_t>(bone) + 1U);
    auto& track = tracks[static_cast<size_t>(bone)];
    if (track.size() >= kMaxKeyframesPerBone) return;
    size_t total = 0U;
    for (const auto& t : tracks) total += t.size();
    if (total >= kMaxTotalKeyframes) return;

    auto it = std::lower_bound(track.begin(), track.end(), frame.time,
                               [](const Keyframe& value, float time) { return value.time < time; });
    if (it != track.end() && std::fabs(it->time - frame.time) <= 1.0e-6F) {
        *it = frame;
    } else {
        track.insert(it, frame);
    }
    duration = std::max(duration, frame.time);
}

const std::vector<Keyframe>& AnimationClip::GetFrames(int bone) const {
    static const std::vector<Keyframe> empty;
    if (bone < 0 || static_cast<size_t>(bone) >= tracks.size()) return empty;
    return tracks[static_cast<size_t>(bone)];
}

float AnimationClip::GetDuration() const { return duration; }

bool AnimationClip::Sample(int bone, float time, Mat4& out) const {
    const auto& track = GetFrames(bone);
    if (track.empty() || !std::isfinite(time) || !std::isfinite(duration)) return false;
    if (track.size() == 1U || time <= track.front().time) { out = track.front().transform; return true; }
    if (time >= track.back().time) { out = track.back().transform; return true; }

    auto upper = std::upper_bound(track.begin(), track.end(), time,
                                  [](float value, const Keyframe& frame) { return value < frame.time; });
    const auto& b = *upper;
    const auto& a = *(upper - 1);
    const float span = b.time - a.time;
    if (!(span > 0.0F)) return false;
    const float alpha = std::clamp((time - a.time) / span, 0.0F, 1.0F);
    Mat4 candidate = LerpMatrix(a.transform, b.transform, alpha);
    if (!FiniteMatrix(candidate)) return false;
    out = candidate;
    return true;
}

} // namespace NeoEngine
