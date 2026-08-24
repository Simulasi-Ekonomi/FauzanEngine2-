#pragma once

#include <vector>

class OcclusionBuffer
{
public:

    OcclusionBuffer(int width, int height);

    void Clear();

    float GetDepth(int x, int y) const;

    void SetDepth(int x, int y, float depth);

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

private:

    [[maybe_unused]] int width;
    [[maybe_unused]] int height;

    [[maybe_unused]] std::vector<float> depthBuffer;
};
