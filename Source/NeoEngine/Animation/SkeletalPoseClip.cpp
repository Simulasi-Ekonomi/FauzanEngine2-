#include "SkeletalPoseClip.h"

#include <cmath>

namespace NeoEngine {
namespace {
constexpr float kEpsilon = 0.000001F;
bool Finite(const float value) { return std::isfinite(value); }
bool FiniteVec3(const PoseVec3& value) { return Finite(value.x) && Finite(value.y) && Finite(value.z); }
bool Normalize(const PoseQuat& source, PoseQuat& result) {
    const float lengthSquared = source.x * source.x + source.y * source.y + source.z * source.z + source.w * source.w;
    if (!Finite(source.x) || !Finite(source.y) || !Finite(source.z) || !Finite(source.w) || !Finite(lengthSquared) || lengthSquared < kEpsilon * kEpsilon) return false;
    const float inverseLength = 1.0F / std::sqrt(lengthSquared);
    result = {source.x * inverseLength, source.y * inverseLength, source.z * inverseLength, source.w * inverseLength};
    return Finite(result.x) && Finite(result.y) && Finite(result.z) && Finite(result.w);
}
bool ValidKey(const SkeletalPoseKeyframe& key) {
    PoseQuat normalized{};
    return Finite(key.time) && key.time >= 0.0F && FiniteVec3(key.translation) && Normalize(key.rotation, normalized) && FiniteVec3(key.scale) && std::fabs(key.scale.x) >= kEpsilon && std::fabs(key.scale.y) >= kEpsilon && std::fabs(key.scale.z) >= kEpsilon;
}
PoseVec3 Lerp(const PoseVec3& left, const PoseVec3& right, const float alpha) { return {left.x + (right.x - left.x) * alpha, left.y + (right.y - left.y) * alpha, left.z + (right.z - left.z) * alpha}; }
bool Nlerp(const PoseQuat& left, const PoseQuat& right, const float alpha, PoseQuat& result) {
    PoseQuat a{}, b{};
    if (!Normalize(left, a) || !Normalize(right, b)) return false;
    if (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w < 0.0F) b = {-b.x, -b.y, -b.z, -b.w};
    return Normalize({a.x + (b.x - a.x) * alpha, a.y + (b.y - a.y) * alpha, a.z + (b.z - a.z) * alpha, a.w + (b.w - a.w) * alpha}, result);
}
bool ComposeAffine(const PoseVec3& translation, const PoseQuat& rotation, const PoseVec3& scale, Mat4& matrix) {
    PoseQuat q{};
    if (!FiniteVec3(translation) || !FiniteVec3(scale) || std::fabs(scale.x) < kEpsilon || std::fabs(scale.y) < kEpsilon || std::fabs(scale.z) < kEpsilon || !Normalize(rotation, q)) return false;
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    matrix = {};
    matrix.m[0] = (1.0F - 2.0F * (yy + zz)) * scale.x; matrix.m[1] = 2.0F * (xy + wz) * scale.x; matrix.m[2] = 2.0F * (xz - wy) * scale.x;
    matrix.m[4] = 2.0F * (xy - wz) * scale.y; matrix.m[5] = (1.0F - 2.0F * (xx + zz)) * scale.y; matrix.m[6] = 2.0F * (yz + wx) * scale.y;
    matrix.m[8] = 2.0F * (xz + wy) * scale.z; matrix.m[9] = 2.0F * (yz - wx) * scale.z; matrix.m[10] = (1.0F - 2.0F * (xx + yy)) * scale.z;
    matrix.m[12] = translation.x; matrix.m[13] = translation.y; matrix.m[14] = translation.z; matrix.m[15] = 1.0F;
    for (const float value : matrix.m) if (!Finite(value)) return false;
    return true;
}
bool SampleTrackPose(const std::vector<SkeletalPoseKeyframe>& keys, const float time, SkeletalPoseKeyframe& output) {
    const SkeletalPoseKeyframe* left = &keys.front(); const SkeletalPoseKeyframe* right = left; float alpha = 0.0F;
    if (time >= keys.back().time) { left = &keys.back(); right = left; }
    else for (size_t index = 1U; index < keys.size(); ++index) if (time <= keys[index].time) { right = &keys[index]; left = &keys[index - 1U]; alpha = (time - left->time) / (right->time - left->time); break; }
    PoseQuat rotation{};
    if (!Nlerp(left->rotation, right->rotation, alpha, rotation)) return false;
    output = {}; output.translation = Lerp(left->translation, right->translation, alpha); output.rotation = rotation; output.scale = Lerp(left->scale, right->scale, alpha); return true;
}
bool SampleTrack(const std::vector<SkeletalPoseKeyframe>& keys, const float time, Mat4& output) {
    SkeletalPoseKeyframe pose{};
    return SampleTrackPose(keys, time, pose) && ComposeAffine(pose.translation, pose.rotation, pose.scale, output);
}
} // namespace

bool SkeletalPoseClip::Configure(const size_t boneCount) {
    if (boneCount == 0U || boneCount > kMaxBones) { lastError_ = SkeletalPoseError::BoneCountExceeded; return false; }
    std::vector<std::vector<SkeletalPoseKeyframe>> candidate(boneCount);
    tracks_.swap(candidate); lastError_ = SkeletalPoseError::None; return true;
}

bool SkeletalPoseClip::SetTrack(const size_t boneIndex, const std::vector<SkeletalPoseKeyframe>& keys) {
    if (tracks_.empty()) { lastError_ = SkeletalPoseError::NotConfigured; return false; }
    if (boneIndex >= tracks_.size()) { lastError_ = SkeletalPoseError::InvalidTrack; return false; }
    if (!tracks_[boneIndex].empty()) { lastError_ = SkeletalPoseError::DuplicateTrack; return false; }
    if (keys.empty() || keys.size() > kMaxKeysPerTrack || !ValidKey(keys.front()) || std::fabs(keys.front().time) > kEpsilon) { lastError_ = SkeletalPoseError::InvalidKey; return false; }
    for (size_t index = 1U; index < keys.size(); ++index) if (!ValidKey(keys[index]) || keys[index].time <= keys[index - 1U].time) { lastError_ = SkeletalPoseError::InvalidKey; return false; }
    tracks_[boneIndex] = keys; lastError_ = SkeletalPoseError::None; return true;
}

bool SkeletalPoseClip::Sample(const float time, std::vector<Mat4>& output) {
    if (tracks_.empty()) { lastError_ = SkeletalPoseError::NotConfigured; return false; }
    if (!Finite(time) || time < 0.0F) { lastError_ = SkeletalPoseError::InvalidSampleTime; return false; }
    std::vector<Mat4> candidate; candidate.reserve(tracks_.size());
    for (const auto& track : tracks_) {
        if (track.empty()) { lastError_ = SkeletalPoseError::IncompleteClip; return false; }
        Mat4 local{}; if (!SampleTrack(track, time, local)) { lastError_ = SkeletalPoseError::InvalidPose; return false; }
        candidate.push_back(local);
    }
    output.swap(candidate); lastError_ = SkeletalPoseError::None; return true;
}

float SkeletalPoseClip::Duration() const {
    float duration = 0.0F;
    for (const auto& track : tracks_) {
        if (track.empty()) return 0.0F;
        duration = std::max(duration, track.back().time);
    }
    return duration;
}

bool SkeletalPoseClip::SampleLooped(const float time, std::vector<Mat4>& output) {
    if (tracks_.empty()) { lastError_ = SkeletalPoseError::NotConfigured; return false; }
    if (!Finite(time) || time < 0.0F) { lastError_ = SkeletalPoseError::InvalidSampleTime; return false; }
    for (const auto& track : tracks_) if (track.empty()) { lastError_ = SkeletalPoseError::IncompleteClip; return false; }
    const float duration = Duration();
    if (!Finite(duration) || duration < kEpsilon) { lastError_ = SkeletalPoseError::LoopUnavailable; return false; }
    return Sample(std::fmod(time, duration), output);
}

bool SkeletalPoseClip::SampleBlended(const SkeletalPoseClip& other, const float time, const float otherTime, const float blendFactor, std::vector<Mat4>& output) {
    if (tracks_.empty()) { lastError_ = SkeletalPoseError::NotConfigured; return false; }
    if (!Finite(time) || time < 0.0F || !Finite(otherTime) || otherTime < 0.0F) { lastError_ = SkeletalPoseError::InvalidSampleTime; return false; }
    if (!Finite(blendFactor) || blendFactor < 0.0F || blendFactor > 1.0F) { lastError_ = SkeletalPoseError::InvalidBlendFactor; return false; }
    if (tracks_.size() != other.tracks_.size()) { lastError_ = SkeletalPoseError::BlendIncompatible; return false; }
    std::vector<Mat4> candidate; candidate.reserve(tracks_.size());
    for (size_t index = 0U; index < tracks_.size(); ++index) {
        if (tracks_[index].empty() || other.tracks_[index].empty()) { lastError_ = SkeletalPoseError::IncompleteClip; return false; }
        SkeletalPoseKeyframe left{}, right{}; PoseQuat rotation{}; Mat4 matrix{};
        if (!SampleTrackPose(tracks_[index], time, left) || !SampleTrackPose(other.tracks_[index], otherTime, right) || !Nlerp(left.rotation, right.rotation, blendFactor, rotation) || !ComposeAffine(Lerp(left.translation, right.translation, blendFactor), rotation, Lerp(left.scale, right.scale, blendFactor), matrix)) { lastError_ = SkeletalPoseError::InvalidPose; return false; }
        candidate.push_back(matrix);
    }
    output.swap(candidate); lastError_ = SkeletalPoseError::None; return true;
}

} // namespace NeoEngine
