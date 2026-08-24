#pragma once

#include <cstdint>
#include <vector>
#include "Bone.h"

namespace NeoEngine
{

struct VertexWeight
{
    int boneIDs[4] = {-1, -1, -1, -1};
    float weights[4] = {0.0F, 0.0F, 0.0F, 0.0F};
};

enum class SkinningError : uint8_t { None, InvalidVertices, WeightCountMismatch, BoneCapacityExceeded, InvalidWeight, InvalidBoneIndex, WeightSumInvalid, InvalidMatrix, TransformInvalid, NormalCountMismatch, InvalidNormal, NormalTransformInvalid };

class Skinning
{
public:
    static constexpr size_t kMaxVertices = 4096U;
    static constexpr size_t kMaxBones = 128U;

    // Applies four-influence linear-blend skinning to xyz-only CPU positions atomically.
    [[nodiscard]] static bool ApplySkinning(
        std::vector<float>& vertices,
        const std::vector<VertexWeight>& weights,
        const std::vector<Mat4>& boneMatrices,
        SkinningError* error = nullptr
    );
    // Applies affine inverse-transpose normal skinning with the same four influences and atomic outputs.
    [[nodiscard]] static bool ApplySkinningWithNormals(
        std::vector<float>& vertices,
        std::vector<float>& normals,
        const std::vector<VertexWeight>& weights,
        const std::vector<Mat4>& boneMatrices,
        SkinningError* error = nullptr
    );

};

}
