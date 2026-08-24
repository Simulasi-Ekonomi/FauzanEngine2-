#pragma once

#include <vector>

class HiZBuffer
{
public:

    HiZBuffer(int width, int height);

    void Build(const std::vector<float>& depth);

    float Sample(int level, int x, int y) const;

    int GetLevels() const { return levels; }

private:

    [[maybe_unused]] int width;
    [[maybe_unused]] int height;
    [[maybe_unused]] int levels;

    [[maybe_unused]] std::vector<std::vector<float>> mip;
};
