#include "SkinningGPU.h"
#include <cmath>
#include <algorithm>
void SkinningGPU::UploadBones(const std::vector<std::array<float,16>>& matrices) {
    palette.clear(); palette.reserve(std::min<size_t>(matrices.size(),256));
    for(size_t i=0;i<matrices.size() && i<256;++i){bool finite=true;for(float v:matrices[i])finite&=std::isfinite(v);if(!finite){palette.clear();bound=false;return;}palette.push_back(matrices[i]);}
    bound=false;
}
void SkinningGPU::Bind(){ bound=!palette.empty(); }