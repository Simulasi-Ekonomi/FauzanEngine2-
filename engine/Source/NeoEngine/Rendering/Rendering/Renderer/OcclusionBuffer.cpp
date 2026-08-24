#include <cassert>
#include "OcclusionBuffer.h"
#include <algorithm>

OcclusionBuffer::OcclusionBuffer(int w, int h)
    : width(w), height(h)
{
    depthBuffer.resize(width * height, 1.0f);
}

void OcclusionBuffer::Clear()
{
    std::fill(depthBuffer.begin(), depthBuffer.end(), 1.0f);
}

float OcclusionBuffer::GetDepth(int x, int y) const
{
    return depthBuffer[y * width + x];
}

void OcclusionBuffer::SetDepth(int x, int y, float depth)
{
    depthBuffer[y * width + x] = depth;
}
