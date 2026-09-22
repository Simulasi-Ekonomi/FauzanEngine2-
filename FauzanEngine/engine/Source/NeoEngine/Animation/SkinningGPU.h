#pragma once
#include <vector>
#include <array>
class SkinningGPU{public:void UploadBones(const std::vector<std::array<float,16>>&);void Bind();size_t BoneCount()const{return palette.size();}bool IsBound()const{return bound;}private:std::vector<std::array<float,16>>palette;bool bound=false;};