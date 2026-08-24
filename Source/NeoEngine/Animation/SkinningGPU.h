#pragma once

#include <vector>
#include <array>

class SkinningGPU
{
public:

    void UploadBones(const std::vector<std::array<float,16>>& matrices);

    void Bind();

private:

    [[maybe_unused]] unsigned int boneBuffer;
};
