#include "Skeleton.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine
{
namespace {
bool Finite(const Mat4& matrix) { return std::all_of(std::begin(matrix.m), std::end(matrix.m), [](float value) { return std::isfinite(value); }); }
bool AffineFinite(const Mat4& matrix) { return Finite(matrix) && std::fabs(matrix.m[3]) <= 0.00001F && std::fabs(matrix.m[7]) <= 0.00001F && std::fabs(matrix.m[11]) <= 0.00001F && std::fabs(matrix.m[15] - 1.0F) <= 0.00001F; }
bool InvertAffine(const Mat4& source, Mat4& result) {
    if (!AffineFinite(source)) return false;
    const float a = source.m[0], b = source.m[4], c = source.m[8];
    const float d = source.m[1], e = source.m[5], f = source.m[9];
    const float g = source.m[2], h = source.m[6], i = source.m[10];
    const float determinant = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (!std::isfinite(determinant) || std::fabs(determinant) < 0.000001F) return false;
    const float inverseDeterminant = 1.0F / determinant;
    result = {};
    result.m[0] = (e * i - f * h) * inverseDeterminant; result.m[4] = (c * h - b * i) * inverseDeterminant; result.m[8] = (b * f - c * e) * inverseDeterminant;
    result.m[1] = (f * g - d * i) * inverseDeterminant; result.m[5] = (a * i - c * g) * inverseDeterminant; result.m[9] = (c * d - a * f) * inverseDeterminant;
    result.m[2] = (d * h - e * g) * inverseDeterminant; result.m[6] = (b * g - a * h) * inverseDeterminant; result.m[10] = (a * e - b * d) * inverseDeterminant;
    result.m[15] = 1.0F;
    result.m[12] = -(result.m[0] * source.m[12] + result.m[4] * source.m[13] + result.m[8] * source.m[14]);
    result.m[13] = -(result.m[1] * source.m[12] + result.m[5] * source.m[13] + result.m[9] * source.m[14]);
    result.m[14] = -(result.m[2] * source.m[12] + result.m[6] * source.m[13] + result.m[10] * source.m[14]);
    return Finite(result);
}
} // namespace

bool Skeleton::TryAddBone(const Bone& bone)
{
    if (bones_.size() >= kMaxBones) { lastError_ = SkeletonError::CapacityExceeded; return false; }
    if (bone.GetName().empty() || bone.GetName().size() > 96U) { lastError_ = SkeletonError::EmptyName; return false; }
    if (std::any_of(bones_.begin(), bones_.end(), [&bone](const Bone& existing) { return existing.GetName() == bone.GetName(); })) { lastError_ = SkeletonError::DuplicateName; return false; }
    const int parent = bone.GetParent();
    if (bones_.empty()) {
        if (parent != -1) { lastError_ = SkeletonError::RootRequired; return false; }
        rootIndex_ = 0;
    } else {
        if (parent == -1) { lastError_ = SkeletonError::MultipleRoots; return false; }
        if (parent < 0 || static_cast<size_t>(parent) >= bones_.size()) { lastError_ = SkeletonError::InvalidParent; return false; }
    }
    bones_.push_back(bone);
    inverseBindReady_ = false;
    lastError_ = SkeletonError::None;
    return true;
}

const std::vector<Bone>& Skeleton::GetBones() const
{
    return bones_;
}

const Bone* Skeleton::GetBone(size_t index) const
{
    return index < bones_.size() ? &bones_[index] : nullptr;
}

size_t Skeleton::GetBoneCount() const
{
    return bones_.size();
}

bool Skeleton::EvaluateGlobalBindPose(std::vector<Mat4>& output)
{
    if (bones_.empty() || rootIndex_ != 0) { lastError_ = SkeletonError::EmptySkeleton; return false; }
    std::vector<Mat4> candidate;
    candidate.reserve(bones_.size());
    for (size_t index = 0U; index < bones_.size(); ++index) {
        const Mat4& local = bones_[index].localBindPose;
        if (!Finite(local)) { lastError_ = SkeletonError::InvalidBindPose; return false; }
        Mat4 global{};
        if (index == 0U) global = local;
        else {
            const int parent = bones_[index].GetParent();
            if (parent < 0 || static_cast<size_t>(parent) >= candidate.size()) { lastError_ = SkeletonError::InvalidParent; return false; }
            global = MulMat4(candidate[static_cast<size_t>(parent)], local);
        }
        if (!Finite(global)) { lastError_ = SkeletonError::InvalidBindPose; return false; }
        candidate.push_back(global);
    }
    output.swap(candidate);
    lastError_ = SkeletonError::None;
    return true;
}

bool Skeleton::DeriveInverseBindPose()
{
    std::vector<Mat4> global;
    if (!EvaluateGlobalBindPose(global)) return false;
    std::vector<Mat4> inverse;
    inverse.reserve(global.size());
    for (const Mat4& matrix : global) {
        Mat4 candidate{};
        if (!InvertAffine(matrix, candidate)) { lastError_ = SkeletonError::SingularBindPose; return false; }
        inverse.push_back(candidate);
    }
    for (size_t index = 0U; index < bones_.size(); ++index) bones_[index].inverseBindPose = inverse[index];
    inverseBindReady_ = true;
    lastError_ = SkeletonError::None;
    return true;
}

bool Skeleton::EvaluateSkinningPalette(const std::vector<Mat4>& localPose, std::vector<Mat4>& output)
{
    if (!inverseBindReady_) { lastError_ = SkeletonError::InverseBindUnavailable; return false; }
    if (localPose.size() != bones_.size()) { lastError_ = SkeletonError::PoseCountMismatch; return false; }
    std::vector<Mat4> global;
    std::vector<Mat4> palette;
    global.reserve(bones_.size()); palette.reserve(bones_.size());
    for (size_t index = 0U; index < bones_.size(); ++index) {
        if (!AffineFinite(localPose[index])) { lastError_ = SkeletonError::InvalidPose; return false; }
        Mat4 world{};
        if (index == 0U) world = localPose[index];
        else {
            const int parent = bones_[index].GetParent();
            if (parent < 0 || static_cast<size_t>(parent) >= global.size()) { lastError_ = SkeletonError::InvalidParent; return false; }
            world = MulMat4(global[static_cast<size_t>(parent)], localPose[index]);
        }
        const Mat4 skinning = MulMat4(world, bones_[index].inverseBindPose);
        if (!AffineFinite(world) || !AffineFinite(skinning)) { lastError_ = SkeletonError::InvalidPose; return false; }
        global.push_back(world); palette.push_back(skinning);
    }
    output.swap(palette);
    lastError_ = SkeletonError::None;
    return true;
}

}
