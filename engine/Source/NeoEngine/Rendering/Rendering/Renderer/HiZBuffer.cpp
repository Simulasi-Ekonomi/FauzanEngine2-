#include <cassert>
#include "HiZBuffer.h"
#include <algorithm>

HiZBuffer::HiZBuffer(int w, int h)
{
    width = w;
    height = h;

    levels = 0;

    int size = std::max(width, height);

    while(size > 1)
    {
        size >>= 1;
        levels++;
    }

    mip.resize(levels);
}

void HiZBuffer::Build(const std::vector<float>& depth)
{
    mip[0] = depth;

    for(int level = 1; level < levels; level++)
    {
        int prevSize = mip[level - 1].size() / 4;

        mip[level].resize(prevSize);

        for(int i = 0; i < prevSize; i++)
        {
            float a = mip[level - 1][i * 4 + 0];
            float b = mip[level - 1][i * 4 + 1];
            float c = mip[level - 1][i * 4 + 2];
            float d = mip[level - 1][i * 4 + 3];

            mip[level][i] = std::max(std::max(a,b), std::max(c,d));
        }
    }
}

float HiZBuffer::Sample(int level, int x, int y) const
{
    return mip[level][y + x];
}
