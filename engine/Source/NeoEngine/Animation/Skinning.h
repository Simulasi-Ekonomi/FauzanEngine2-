#pragma once

#include <vector>
#include "Bone.h"

namespace NeoEngine
{

struct VertexWeight
{
    int boneIDs[4];
    float weights[4];
};

class Skinning
{
public:

    static void ApplySkinning(
        std::vector<float>& vertices,
        const std::vector<VertexWeight>& weights,
        const std::vector<Mat4>& boneMatrices
    );

};

}
