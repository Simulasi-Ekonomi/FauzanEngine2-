#include "Skinning.h"

#include <cmath>

namespace NeoEngine {
namespace {
void SetError(SkinningError* error, SkinningError value) { if (error != nullptr) *error = value; }
bool FiniteMatrix(const Mat4& matrix) { for (float value : matrix.m) if (!std::isfinite(value)) return false; return true; }
bool TransformPosition(const Mat4& matrix, const float* source, float* result) {
    const float x = matrix.m[0] * source[0] + matrix.m[4] * source[1] + matrix.m[8] * source[2] + matrix.m[12];
    const float y = matrix.m[1] * source[0] + matrix.m[5] * source[1] + matrix.m[9] * source[2] + matrix.m[13];
    const float z = matrix.m[2] * source[0] + matrix.m[6] * source[1] + matrix.m[10] * source[2] + matrix.m[14];
    const float w = matrix.m[3] * source[0] + matrix.m[7] * source[1] + matrix.m[11] * source[2] + matrix.m[15];
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(w) || std::fabs(w) < 0.000001F) return false;
    result[0] = x / w; result[1] = y / w; result[2] = z / w;
    return std::isfinite(result[0]) && std::isfinite(result[1]) && std::isfinite(result[2]);
}
struct NormalTransform { float m[9]; };
bool BuildNormalTransform(const Mat4& matrix, NormalTransform& result) {
    if (std::fabs(matrix.m[3]) > 0.00001F || std::fabs(matrix.m[7]) > 0.00001F || std::fabs(matrix.m[11]) > 0.00001F || std::fabs(matrix.m[15] - 1.0F) > 0.00001F) return false;
    const float a = matrix.m[0], b = matrix.m[4], c = matrix.m[8];
    const float d = matrix.m[1], e = matrix.m[5], f = matrix.m[9];
    const float g = matrix.m[2], h = matrix.m[6], i = matrix.m[10];
    const float determinant = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (!std::isfinite(determinant) || std::fabs(determinant) < 0.000001F) return false;
    const float inverse = 1.0F / determinant;
    result.m[0] = (e * i - f * h) * inverse; result.m[1] = (f * g - d * i) * inverse; result.m[2] = (d * h - e * g) * inverse;
    result.m[3] = (c * h - b * i) * inverse; result.m[4] = (a * i - c * g) * inverse; result.m[5] = (b * g - a * h) * inverse;
    result.m[6] = (b * f - c * e) * inverse; result.m[7] = (c * d - a * f) * inverse; result.m[8] = (a * e - b * d) * inverse;
    for (const float value : result.m) if (!std::isfinite(value)) return false;
    return true;
}
bool TransformNormal(const NormalTransform& matrix, const float* source, float* result) {
    result[0] = matrix.m[0] * source[0] + matrix.m[1] * source[1] + matrix.m[2] * source[2];
    result[1] = matrix.m[3] * source[0] + matrix.m[4] * source[1] + matrix.m[5] * source[2];
    result[2] = matrix.m[6] * source[0] + matrix.m[7] * source[1] + matrix.m[8] * source[2];
    return std::isfinite(result[0]) && std::isfinite(result[1]) && std::isfinite(result[2]);
}
} // namespace

bool Skinning::ApplySkinning(std::vector<float>& vertices, const std::vector<VertexWeight>& weights, const std::vector<Mat4>& boneMatrices, SkinningError* error) {
    SetError(error, SkinningError::None);
    if (vertices.empty() || vertices.size() % 3U != 0U || vertices.size() / 3U > kMaxVertices) { SetError(error, SkinningError::InvalidVertices); return false; }
    const size_t vertexCount = vertices.size() / 3U;
    if (weights.size() != vertexCount) { SetError(error, SkinningError::WeightCountMismatch); return false; }
    if (boneMatrices.empty() || boneMatrices.size() > kMaxBones) { SetError(error, SkinningError::BoneCapacityExceeded); return false; }
    for (const Mat4& matrix : boneMatrices) if (!FiniteMatrix(matrix)) { SetError(error, SkinningError::InvalidMatrix); return false; }

    std::vector<float> candidate(vertices);
    for (size_t vertex = 0U; vertex < vertexCount; ++vertex) {
        const float* source = vertices.data() + vertex * 3U;
        if (!std::isfinite(source[0]) || !std::isfinite(source[1]) || !std::isfinite(source[2])) { SetError(error, SkinningError::InvalidVertices); return false; }
        const VertexWeight& influence = weights[vertex];
        float totalWeight = 0.0F;
        float blended[3] = {0.0F, 0.0F, 0.0F};
        for (size_t slot = 0U; slot < 4U; ++slot) {
            const float weight = influence.weights[slot];
            if (!std::isfinite(weight) || weight < 0.0F) { SetError(error, SkinningError::InvalidWeight); return false; }
            if (weight == 0.0F) continue;
            const int boneId = influence.boneIDs[slot];
            if (boneId < 0 || static_cast<size_t>(boneId) >= boneMatrices.size()) { SetError(error, SkinningError::InvalidBoneIndex); return false; }
            float transformed[3];
            if (!TransformPosition(boneMatrices[static_cast<size_t>(boneId)], source, transformed)) { SetError(error, SkinningError::TransformInvalid); return false; }
            totalWeight += weight;
            blended[0] += transformed[0] * weight; blended[1] += transformed[1] * weight; blended[2] += transformed[2] * weight;
        }
        if (!std::isfinite(totalWeight) || std::fabs(totalWeight - 1.0F) > 0.0001F) { SetError(error, SkinningError::WeightSumInvalid); return false; }
        if (!std::isfinite(blended[0]) || !std::isfinite(blended[1]) || !std::isfinite(blended[2])) { SetError(error, SkinningError::TransformInvalid); return false; }
        candidate[vertex * 3U] = blended[0]; candidate[vertex * 3U + 1U] = blended[1]; candidate[vertex * 3U + 2U] = blended[2];
    }
    vertices.swap(candidate);
    return true;
}

bool Skinning::ApplySkinningWithNormals(std::vector<float>& vertices, std::vector<float>& normals, const std::vector<VertexWeight>& weights, const std::vector<Mat4>& boneMatrices, SkinningError* error) {
    SetError(error, SkinningError::None);
    if (vertices.empty() || vertices.size() % 3U != 0U || vertices.size() / 3U > kMaxVertices) { SetError(error, SkinningError::InvalidVertices); return false; }
    const size_t vertexCount = vertices.size() / 3U;
    if (normals.size() != vertices.size()) { SetError(error, SkinningError::NormalCountMismatch); return false; }
    if (weights.size() != vertexCount) { SetError(error, SkinningError::WeightCountMismatch); return false; }
    if (boneMatrices.empty() || boneMatrices.size() > kMaxBones) { SetError(error, SkinningError::BoneCapacityExceeded); return false; }
    std::vector<NormalTransform> normalMatrices; normalMatrices.reserve(boneMatrices.size());
    for (const Mat4& matrix : boneMatrices) {
        if (!FiniteMatrix(matrix)) { SetError(error, SkinningError::InvalidMatrix); return false; }
        NormalTransform normalMatrix{};
        if (!BuildNormalTransform(matrix, normalMatrix)) { SetError(error, SkinningError::NormalTransformInvalid); return false; }
        normalMatrices.push_back(normalMatrix);
    }
    std::vector<float> candidateVertices(vertices); std::vector<float> candidateNormals(normals);
    for (size_t vertex = 0U; vertex < vertexCount; ++vertex) {
        const float* sourcePosition = vertices.data() + vertex * 3U;
        const float* sourceNormal = normals.data() + vertex * 3U;
        const float sourceNormalLengthSquared = sourceNormal[0] * sourceNormal[0] + sourceNormal[1] * sourceNormal[1] + sourceNormal[2] * sourceNormal[2];
        if (!std::isfinite(sourcePosition[0]) || !std::isfinite(sourcePosition[1]) || !std::isfinite(sourcePosition[2])) { SetError(error, SkinningError::InvalidVertices); return false; }
        if (!std::isfinite(sourceNormal[0]) || !std::isfinite(sourceNormal[1]) || !std::isfinite(sourceNormal[2]) || !std::isfinite(sourceNormalLengthSquared) || sourceNormalLengthSquared < 0.000000000001F) { SetError(error, SkinningError::InvalidNormal); return false; }
        const VertexWeight& influence = weights[vertex];
        float totalWeight = 0.0F; float blendedPosition[3] = {0.0F, 0.0F, 0.0F}; float blendedNormal[3] = {0.0F, 0.0F, 0.0F};
        for (size_t slot = 0U; slot < 4U; ++slot) {
            const float weight = influence.weights[slot];
            if (!std::isfinite(weight) || weight < 0.0F) { SetError(error, SkinningError::InvalidWeight); return false; }
            if (weight == 0.0F) continue;
            const int boneId = influence.boneIDs[slot];
            if (boneId < 0 || static_cast<size_t>(boneId) >= boneMatrices.size()) { SetError(error, SkinningError::InvalidBoneIndex); return false; }
            const size_t bone = static_cast<size_t>(boneId); float transformedPosition[3]; float transformedNormal[3];
            if (!TransformPosition(boneMatrices[bone], sourcePosition, transformedPosition) || !TransformNormal(normalMatrices[bone], sourceNormal, transformedNormal)) { SetError(error, SkinningError::TransformInvalid); return false; }
            totalWeight += weight;
            for (size_t component = 0U; component < 3U; ++component) { blendedPosition[component] += transformedPosition[component] * weight; blendedNormal[component] += transformedNormal[component] * weight; }
        }
        const float normalLengthSquared = blendedNormal[0] * blendedNormal[0] + blendedNormal[1] * blendedNormal[1] + blendedNormal[2] * blendedNormal[2];
        if (!std::isfinite(totalWeight) || std::fabs(totalWeight - 1.0F) > 0.0001F) { SetError(error, SkinningError::WeightSumInvalid); return false; }
        if (!std::isfinite(blendedPosition[0]) || !std::isfinite(blendedPosition[1]) || !std::isfinite(blendedPosition[2]) || !std::isfinite(normalLengthSquared) || normalLengthSquared < 0.000000000001F) { SetError(error, SkinningError::TransformInvalid); return false; }
        const float inverseNormalLength = 1.0F / std::sqrt(normalLengthSquared);
        for (size_t component = 0U; component < 3U; ++component) { candidateVertices[vertex * 3U + component] = blendedPosition[component]; candidateNormals[vertex * 3U + component] = blendedNormal[component] * inverseNormalLength; }
    }
    vertices.swap(candidateVertices); normals.swap(candidateNormals);
    return true;
}

} // namespace NeoEngine
