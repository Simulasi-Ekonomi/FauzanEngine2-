#include "HiZBuffer.h"
#include <algorithm>
#include <cstddef>
#include <limits>

HiZBuffer::HiZBuffer(int w, int h) : width(w), height(h) {
    if (width <= 0 || height <= 0) {
        width = height = levels = 0;
        return;
    }
    int levelWidth = width;
    int levelHeight = height;
    while (true) {
        ++levels;
        mipWidths.push_back(levelWidth);
        mipHeights.push_back(levelHeight);
        if (levelWidth == 1 && levelHeight == 1) break;
        levelWidth = std::max(1, (levelWidth + 1) / 2);
        levelHeight = std::max(1, (levelHeight + 1) / 2);
    }
    mip.resize(static_cast<std::size_t>(levels));
    occlusionMip.resize(static_cast<std::size_t>(levels));
}

void HiZBuffer::Build(const std::vector<float>& depth) {
    if (levels == 0 || depth.size() != static_cast<std::size_t>(width * height)) return;
    mip[0] = depth;
    for (int level = 1; level < levels; ++level) {
        const int previousWidth = mipWidths[level - 1];
        const int previousHeight = mipHeights[level - 1];
        const int currentWidth = mipWidths[level];
        const int currentHeight = mipHeights[level];
        mip[level].assign(static_cast<std::size_t>(currentWidth * currentHeight), 0.0F);
        for (int y = 0; y < currentHeight; ++y) {
            for (int x = 0; x < currentWidth; ++x) {
                float value = 0.0F;
                for (int oy = 0; oy < 2; ++oy) {
                    for (int ox = 0; ox < 2; ++ox) {
                        const int sampleX = std::min(previousWidth - 1, x * 2 + ox);
                        const int sampleY = std::min(previousHeight - 1, y * 2 + oy);
                        value = std::max(value, mip[level - 1][static_cast<std::size_t>(sampleY * previousWidth + sampleX)]);
                    }
                }
                mip[level][static_cast<std::size_t>(y * currentWidth + x)] = value;
            }
        }
    }
}

void HiZBuffer::BuildOcclusion(const std::vector<float>& depth) {
    if (levels == 0 || depth.size() != static_cast<std::size_t>(width * height)) return;
    // Reverse-Z convention: clear/far is 0 and larger depth is nearer.
    // MIN reduction is conservative for an entire screen-space rectangle:
    // an object is only rejected when every covered Hi-Z cell is nearer.
    occlusionMip[0] = depth;
    for (int level = 1; level < levels; ++level) {
        const int previousWidth = mipWidths[level - 1];
        const int previousHeight = mipHeights[level - 1];
        const int currentWidth = mipWidths[level];
        const int currentHeight = mipHeights[level];
        occlusionMip[level].assign(static_cast<std::size_t>(currentWidth * currentHeight), 1.0F);
        for (int y = 0; y < currentHeight; ++y) {
            for (int x = 0; x < currentWidth; ++x) {
                float value = std::numeric_limits<float>::infinity();
                for (int oy = 0; oy < 2; ++oy) {
                    for (int ox = 0; ox < 2; ++ox) {
                        const int sampleX = std::min(previousWidth - 1, x * 2 + ox);
                        const int sampleY = std::min(previousHeight - 1, y * 2 + oy);
                        value = std::min(value, occlusionMip[level - 1][static_cast<std::size_t>(sampleY * previousWidth + sampleX)]);
                    }
                }
                occlusionMip[level][static_cast<std::size_t>(y * currentWidth + x)] = value;
            }
        }
    }
}

float HiZBuffer::Sample(int level, int x, int y) const {
    if (level < 0 || level >= levels || x < 0 || y < 0 || x >= mipWidths[level] || y >= mipHeights[level]) return 1.0F;
    return mip[level][static_cast<std::size_t>(y * mipWidths[level] + x)];
}

float HiZBuffer::SampleOcclusion(int level, int x, int y) const {
    if (level < 0 || level >= levels || x < 0 || y < 0 || x >= mipWidths[level] || y >= mipHeights[level]) return 0.0F;
    if (occlusionMip.empty() || occlusionMip[level].empty()) return 0.0F;
    return occlusionMip[level][static_cast<std::size_t>(y * mipWidths[level] + x)];
}

int HiZBuffer::GetWidth(int level) const {
    return (level >= 0 && level < levels) ? mipWidths[level] : 0;
}

int HiZBuffer::GetHeight(int level) const {
    return (level >= 0 && level < levels) ? mipHeights[level] : 0;
}
