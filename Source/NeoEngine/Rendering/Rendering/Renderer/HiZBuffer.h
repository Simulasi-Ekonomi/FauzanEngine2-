#pragma once
#include <vector>

class HiZBuffer {
public:
    HiZBuffer(int width, int height);
    // Existing MAX-reduction pyramid preserved for compatibility.
    void Build(const std::vector<float>& depth);
    // Conservative reverse-Z occlusion pyramid: MIN reduction.
    void BuildOcclusion(const std::vector<float>& depth);
    float Sample(int level, int x, int y) const;
    float SampleOcclusion(int level, int x, int y) const;
    int GetLevels() const { return levels; }
    int GetWidth(int level = 0) const;
    int GetHeight(int level = 0) const;
private:
    int width{};
    int height{};
    int levels{};
    std::vector<std::vector<float>> mip;
    std::vector<std::vector<float>> occlusionMip;
    std::vector<int> mipWidths;
    std::vector<int> mipHeights;
};
