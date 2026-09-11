#include "LodManager.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace NeoEngine {

bool LodManager::Register(AssetID meshId, const std::vector<MeshVariant>& variants) noexcept {
    if (meshId.empty() || variants.empty() || variants.size() > 4) {
        lastError_ = true;
        return false;
    }

    try {
        MeshLodData data{
            variants,
            LODThresholds{}  // Default thresholds
        };
        lodData_[meshId] = std::move(data);
    } catch (...) {
        lastError_ = true;
        return false;
    }

    lastError_ = false;
    return true;
}

bool LodManager::SelectLod(AssetID meshId, float cameraDistance, uint8_t& outLodLevel) const noexcept {
    if (!std::isfinite(cameraDistance) || cameraDistance < 0.0f) {
        lastError_ = true;
        return false;
    }

    auto it = lodData_.find(meshId);
    if (it == lodData_.end()) {
        lastError_ = true;
        return false;
    }

    const MeshLodData& data = it->second;
    if (data.variants.empty()) {
        lastError_ = true;
        return false;
    }

    // Select LOD based on distance thresholds
    outLodLevel = 0;
    for (uint32_t i = 0; i < 4; ++i) {
        if (cameraDistance > data.thresholds.distanceMeters[i]) {
            outLodLevel = data.thresholds.lodLevel[i];
        } else {
            break;
        }
    }

    // Clamp to available LOD levels
    outLodLevel = std::min(outLodLevel, static_cast<uint8_t>(data.variants.size() - 1));

    lastError_ = false;
    return true;
}

bool LodManager::SelectTextureMip(uint32_t fullWidth, float cameraDistance, uint32_t& outMipLevel) const noexcept {
    if (fullWidth == 0 || !std::isfinite(cameraDistance) || cameraDistance < 0.0f) {
        lastError_ = true;
        return false;
    }

    // Heuristic: at 1000m distance, use mip level 4 (1/16 resolution)
    // Scale linearly with distance
    const float mipLevelF = (cameraDistance / 1000.f) * 4.f;
    outMipLevel = std::min(static_cast<uint32_t>(mipLevelF), 8u);  // Cap at 8 levels

    lastError_ = false;
    return true;
}

const MeshVariant* LodManager::GetVariant(AssetID meshId, uint8_t lodLevel) const noexcept {
    auto it = lodData_.find(meshId);
    if (it == lodData_.end()) {
        lastError_ = true;
        return nullptr;
    }

    const MeshLodData& data = it->second;
    if (lodLevel >= data.variants.size()) {
        lastError_ = true;
        return nullptr;
    }

    lastError_ = false;
    return &data.variants[lodLevel];
}

} // namespace NeoEngine
