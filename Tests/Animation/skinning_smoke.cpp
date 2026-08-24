#include "Animation/Skinning.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
NeoEngine::Mat4 Translation(float x, float y, float z) {
    NeoEngine::Mat4 matrix{};
    matrix.m[0] = matrix.m[5] = matrix.m[10] = matrix.m[15] = 1.0F;
    matrix.m[12] = x; matrix.m[13] = y; matrix.m[14] = z;
    return matrix;
}
NeoEngine::Mat4 ScaleTranslate(float scaleX, float scaleY, float scaleZ, float x, float y, float z) {
    NeoEngine::Mat4 matrix{};
    matrix.m[0] = scaleX; matrix.m[5] = scaleY; matrix.m[10] = scaleZ; matrix.m[15] = 1.0F;
    matrix.m[12] = x; matrix.m[13] = y; matrix.m[14] = z;
    return matrix;
}
bool Near(float left, float right) { return std::fabs(left - right) < 0.0001F; }
}

int main() {
    using namespace NeoEngine;
    std::vector<float> positions{0.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F};
    const std::vector<VertexWeight> weights{{{0, -1, -1, -1}, {1.0F, 0.0F, 0.0F, 0.0F}}, {{0, 1, -1, -1}, {0.25F, 0.75F, 0.0F, 0.0F}}};
    const std::vector<Mat4> matrices{Translation(2.0F, 0.0F, 0.0F), Translation(0.0F, 4.0F, 0.0F)};
    SkinningError error = SkinningError::None;
    if (!Skinning::ApplySkinning(positions, weights, matrices, &error) || error != SkinningError::None || !Near(positions[0], 2.0F) || !Near(positions[1], 0.0F) || !Near(positions[3], 2.5F) || !Near(positions[4], 3.0F)) return 1;
    const std::vector<float> stable = positions;
    std::vector<VertexWeight> invalidIndex = weights; invalidIndex[0].boneIDs[0] = 9;
    if (Skinning::ApplySkinning(positions, invalidIndex, matrices, &error) || error != SkinningError::InvalidBoneIndex || positions != stable) return 1;
    std::vector<VertexWeight> invalidSum = weights; invalidSum[1].weights[1] = 0.50F;
    if (Skinning::ApplySkinning(positions, invalidSum, matrices, &error) || error != SkinningError::WeightSumInvalid || positions != stable) return 1;
    std::vector<Mat4> invalidMatrix = matrices; invalidMatrix[0].m[0] = std::numeric_limits<float>::quiet_NaN();
    if (Skinning::ApplySkinning(positions, weights, invalidMatrix, &error) || error != SkinningError::InvalidMatrix || positions != stable) return 1;
    if (Skinning::ApplySkinning(positions, std::vector<VertexWeight>{weights[0]}, matrices, &error) || error != SkinningError::WeightCountMismatch || positions != stable) return 1;
    std::vector<float> normalPositions{1.0F, 1.0F, 0.0F}; std::vector<float> normals{1.0F, 1.0F, 0.0F};
    const std::vector<VertexWeight> normalWeights{{{0, -1, -1, -1}, {1.0F, 0.0F, 0.0F, 0.0F}}};
    const std::vector<Mat4> normalMatrices{ScaleTranslate(2.0F, 1.0F, 1.0F, 3.0F, 0.0F, 0.0F)};
    if (!Skinning::ApplySkinningWithNormals(normalPositions, normals, normalWeights, normalMatrices, &error) || error != SkinningError::None || !Near(normalPositions[0], 5.0F) || !Near(normalPositions[1], 1.0F) || !Near(normals[0], 0.4472136F) || !Near(normals[1], 0.8944272F) || !Near(normals[2], 0.0F)) return 1;
    const std::vector<float> stableNormalPositions = normalPositions; const std::vector<float> stableNormals = normals;
    std::vector<float> shortNormals{1.0F, 0.0F};
    if (Skinning::ApplySkinningWithNormals(normalPositions, shortNormals, normalWeights, normalMatrices, &error) || error != SkinningError::NormalCountMismatch || normalPositions != stableNormalPositions || shortNormals != std::vector<float>{1.0F, 0.0F} || normals != stableNormals) return 1;
    std::vector<float> invalidNormals{0.0F, 0.0F, 0.0F};
    if (Skinning::ApplySkinningWithNormals(normalPositions, invalidNormals, normalWeights, normalMatrices, &error) || error != SkinningError::InvalidNormal || normalPositions != stableNormalPositions || invalidNormals != std::vector<float>{0.0F, 0.0F, 0.0F} || normals != stableNormals) return 1;
    std::vector<Mat4> singularNormalMatrix = normalMatrices; singularNormalMatrix[0].m[0] = 0.0F;
    if (Skinning::ApplySkinningWithNormals(normalPositions, normals, normalWeights, singularNormalMatrix, &error) || error != SkinningError::NormalTransformInvalid || normalPositions != stableNormalPositions || normals != stableNormals) return 1;
    std::vector<Mat4> projectiveNormalMatrix = normalMatrices; projectiveNormalMatrix[0].m[3] = 0.25F;
    if (Skinning::ApplySkinningWithNormals(normalPositions, normals, normalWeights, projectiveNormalMatrix, &error) || error != SkinningError::NormalTransformInvalid || normalPositions != stableNormalPositions || normals != stableNormals) return 1;
    std::printf("SKINNING_SMOKE_OK vertices=2 influences=4 blend=1 normals=1 affine=1 atomic=1 validation=1 cpuOnly=1\n");
    return 0;
}
