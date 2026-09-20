#include "HiZBuffer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

HiZBuffer::HiZBuffer(int width, int height)
    : width_(width), height_(height) {
    if (width_ <= 0 || height_ <= 0) {
        throw std::invalid_argument("HiZBuffer dimensions must be positive");
    }
    const int maxDimension = std::max(width_, height_);
    levels_ = 1;
    for (int size = maxDimension; size > 1; size = (size + 1) / 2) {
        ++levels_;
    }
    mip_.resize(static_cast<std::size_t>(levels_));
    occlusionMip_.resize(static_cast<std::size_t>(levels_));
}

int HiZBuffer::LevelExtent(int base, int level) {
    int value = base;
    for (int i = 0; i < level; ++i) value = std::max(1, (value + 1) / 2);
    return value;
}

int HiZBuffer::GetWidth(int level) const {
    if (level < 0 || level >= levels_) return 0;
    return LevelExtent(width_, level);
}

int HiZBuffer::GetHeight(int level) const {
    if (level < 0 || level >= levels_) return 0;
    return LevelExtent(height_, level);
}

void HiZBuffer::BuildPyramid(const std::vector<float>& depth,
                             int width,
                             int height,
                             std::vector<std::vector<float>>& pyramid) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("HiZ pyramid dimensions must be positive");
    }
    const std::size_t expectedSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (depth.size() != expectedSize) {
        throw std::invalid_argument("HiZ depth buffer size does not match dimensions");
    }

    pyramid[0] = depth;
    for (std::size_t level = 1; level < pyramid.size(); ++level) {
        const int prevWidth = LevelExtent(width, static_cast<int>(level - 1));
        const int prevHeight = LevelExtent(height, static_cast<int>(level - 1));
        const int outWidth = LevelExtent(width, static_cast<int>(level));
        const int outHeight = LevelExtent(height, static_cast<int>(level));
        auto& out = pyramid[level];
        const auto& in = pyramid[level - 1];
        out.assign(static_cast<std::size_t>(outWidth * outHeight),
                   std::numeric_limits<float>::infinity());

        for (int y = 0; y < outHeight; ++y) {
            for (int x = 0; x < outWidth; ++x) {
                const int x0 = std::min(2 * x, prevWidth - 1);
                const int x1 = std::min(x0 + 1, prevWidth - 1);
                const int y0 = std::min(2 * y, prevHeight - 1);
                const int y1 = std::min(y0 + 1, prevHeight - 1);
                const float a = in[static_cast<std::size_t>(y0 * prevWidth + x0)];
                const float b = in[static_cast<std::size_t>(y0 * prevWidth + x1)];
                const float c = in[static_cast<std::size_t>(y1 * prevWidth + x0)];
                const float d = in[static_cast<std::size_t>(y1 * prevWidth + x1)];
                out[static_cast<std::size_t>(y * outWidth + x)] =
                    std::min(std::min(a, b), std::min(c, d));
            }
        }
    }
}

void HiZBuffer::Build(const std::vector<float>& depth) {
    BuildPyramid(depth, width_, height_, mip_);
}

void HiZBuffer::BuildOcclusion(const std::vector<float>& depth) {
    // Reversed-Z convention: minimum depth is the conservative occluder value.
    BuildPyramid(depth, width_, height_, occlusionMip_);
}

float HiZBuffer::Sample(int level, int x, int y) const {
    if (level < 0 || level >= levels_) return 0.0F;
    const int width = GetWidth(level);
    const int height = GetHeight(level);
    if (x < 0 || y < 0 || x >= width || y >= height) return 0.0F;
    return mip_[static_cast<std::size_t>(level)]
               [static_cast<std::size_t>(y * width + x)];
}

float HiZBuffer::SampleOcclusion(int level, int x, int y) const {
    if (level < 0 || level >= levels_) return 0.0F;
    const int width = GetWidth(level);
    const int height = GetHeight(level);
    if (x < 0 || y < 0 || x >= width || y >= height) return 0.0F;
    return occlusionMip_[static_cast<std::size_t>(level)]
               [static_cast<std::size_t>(y * width + x)];
}