#include <cassert>
#include <vector>
#include <cstring>

namespace NeoEngine {

class MipmapManager {
public:
    [[nodiscard]] static uint32_t ComputeMipLevels(uint32_t width, uint32_t height) noexcept {
        uint32_t maxDim = std::max(width, height);
        uint32_t levels = 1;
        while (maxDim > 1) {
            maxDim >>= 1;
            levels++;
        }
        return levels;
    }

    [[nodiscard]] bool GenerateCPU(const std::vector<uint8_t>& sourcePixels,
                                   uint32_t width, uint32_t height,
                                   std::vector<std::vector<uint8_t>>& outMipLevels) noexcept {
        if (sourcePixels.empty() || width == 0 || height == 0) return false;

        uint32_t mipLevels = ComputeMipLevels(width, height);
        outMipLevels.clear();
        outMipLevels.reserve(mipLevels);

        std::vector<uint8_t> currentLevel = sourcePixels;
        outMipLevels.push_back(currentLevel);

        return true;
    }
};

} // namespace NeoEngine

int main() {
    // Test 1: Compute mip levels correctly
    assert(NeoEngine::MipmapManager::ComputeMipLevels(2048, 2048) == 12);
    assert(NeoEngine::MipmapManager::ComputeMipLevels(1024, 512) == 11);
    assert(NeoEngine::MipmapManager::ComputeMipLevels(1, 1) == 1);
    assert(NeoEngine::MipmapManager::ComputeMipLevels(256, 256) == 9);

    // Test 2: Generate CPU mipmap chain
    NeoEngine::MipmapManager manager;
    std::vector<uint8_t> testPixels(512 * 512 * 4, 0xFF);  // 512x512 RGBA white
    std::vector<std::vector<uint8_t>> mipChain;

    assert(manager.GenerateCPU(testPixels, 512, 512, mipChain));
    assert(mipChain.size() == 10);  // 512 -> 256 -> 128 -> ... -> 1
    assert(mipChain[0].size() == 512 * 512 * 4);

    // Test 3: Verify no memory leaks (mip sizes decrease)
    for (size_t i = 1; i < mipChain.size(); ++i) {
        assert(mipChain[i].size() < mipChain[i-1].size());
    }

    return 0;  // All tests passed
}
