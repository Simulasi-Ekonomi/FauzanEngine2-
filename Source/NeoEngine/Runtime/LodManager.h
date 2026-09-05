#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace NeoEngine {

using AssetID = std::string;

struct LODThresholds {
    float distanceMeters[4] = {100.f, 500.f, 2000.f, 10000.f};
    uint8_t lodLevel[4] = {0, 1, 2, 3};  // 0=full detail, 3=lowest
};

struct MeshVariant {
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    float screenCoverageThreshold = 0.01f;  // Cull if <1% of screen
    void* gpuBufferHandle = nullptr;
};

class LodManager {
public:
    LodManager() = default;
    ~LodManager() = default;

    LodManager(const LodManager&) = delete;
    LodManager& operator=(const LodManager&) = delete;

    // Register mesh LOD variants
    [[nodiscard]] bool Register(AssetID meshId, const std::vector<MeshVariant>& variants) noexcept;
    
    // Select LOD level based on camera distance
    [[nodiscard]] bool SelectLod(AssetID meshId, float cameraDistance, uint8_t& outLodLevel) const noexcept;
    
    // Select texture mip level based on distance + FOV
    [[nodiscard]] bool SelectTextureMip(uint32_t fullWidth, float cameraDistance, uint32_t& outMipLevel) const noexcept;
    
    // Get variant info
    [[nodiscard]] const MeshVariant* GetVariant(AssetID meshId, uint8_t lodLevel) const noexcept;
    
    [[nodiscard]] bool LastError() const noexcept { return lastError_; }

private:
    struct MeshLodData {
        std::vector<MeshVariant> variants;
        LODThresholds thresholds;
    };

    std::unordered_map<AssetID, MeshLodData> lodData_;
    mutable bool lastError_ = false;
};

} // namespace NeoEngine
