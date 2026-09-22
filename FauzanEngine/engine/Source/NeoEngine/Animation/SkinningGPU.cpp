#include "SkinningGPU.h"
#include <algorithm>

void SkinningGPU::UploadBones(const std::vector<std::array<float,16>>& matrices){
    boneBuffer = static_cast<unsigned int>(matrices.size());
}
void SkinningGPU::Bind(){
    // Binding is represented by the allocated bone-buffer handle; renderer-specific descriptor binding occurs at RHI level.
}
