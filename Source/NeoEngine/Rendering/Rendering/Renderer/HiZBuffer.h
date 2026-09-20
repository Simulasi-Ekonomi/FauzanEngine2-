#pragma once

#include <cstddef>
#include <vector>

class HiZBuffer {
public:
    HiZBuffer(int width, int height);

    void Build(const std::vector<float>& depth);
    void BuildOcclusion(const std::vector<float>& depth);

    float Sample(int level, int x, int y) const;
    float SampleOcclusion(int level, int x, int y) const;

    int GetLevels() const { return levels_; }
    int GetWidth(int level = 0) const;
    int GetHeight(int level = 0) const;

private:
    static int LevelExtent(int base, int level);
    static void BuildPyramid(const std::vector<float>& depth,
                             int width,
                             int height,
                             std::vector<std::vector<float>>& pyramid);

    int width_ = 0;
    int height_ = 0;
    int levels_ = 0;
    std::vector<std::vector<float>> mip_;
    std::vector<std::vector<float>> occlusionMip_;
};