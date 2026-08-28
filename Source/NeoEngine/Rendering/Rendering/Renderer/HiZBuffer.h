#pragma once
#include <vector>

class HiZBuffer {
public:
    HiZBuffer(int width, int height);
    void Build(const std::vector<float>& depth);
    float Sample(int level, int x, int y) const;
    int GetLevels() const { return levels; }
private:
    int width{};
    int height{};
    int levels{};
    std::vector<std::vector<float>> mip;
    std::vector<int> mipWidths;
    std::vector<int> mipHeights;
};
