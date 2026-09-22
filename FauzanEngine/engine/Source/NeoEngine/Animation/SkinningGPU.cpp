#include "SkinningGPU.h"
#include <cmath>
#include <algorithm>
void SkinningGPU::UploadBones(const std::vector<std::array<float,16>>&m){palette.clear();palette.reserve(std::min<size_t>(m.size(),256));for(size_t i=0;i<m.size()&&i<256;++i){bool f=true;for(float v:m[i])f&=std::isfinite(v);if(!f){palette.clear();bound=false;return;}palette.push_back(m[i]);}bound=false;} void SkinningGPU::Bind(){bound=!palette.empty();}