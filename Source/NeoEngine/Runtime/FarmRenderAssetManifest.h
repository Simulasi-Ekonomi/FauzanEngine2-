#pragma once

#include "AssetRegistry.h"
#include "AssetResourceManager.h"
#include "FarmSpriteRenderAdapter.h"

#include <array>
#include <cstdint>

namespace NeoEngine {

enum class FarmRenderAssetManifestError : uint8_t { None, AlreadyBound, InvalidAssetSet, MissingAsset, WrongKind, NotReady, InvalidData, AcquireFailed, ResourceMismatch, Capacity };
struct FarmRenderAssetManifestReceipt { uint16_t assetCount = 0U; uint64_t aggregateContentHash = 0U; bool operator==(const FarmRenderAssetManifestReceipt&) const = default; };

class FarmRenderAssetManifest {
public:
    static constexpr uint16_t kRequiredAssetCount = 16U;
    bool Bind(const FarmSpriteAssetSet& assetSet, const AssetRegistry& registry, AssetResourceManager& resources);
    bool Validate(const AssetRegistry& registry, const AssetResourceManager& resources);
    [[nodiscard]] const FarmSpriteAssetSet& AssetSet() const { return assetSet_; }
    [[nodiscard]] FarmRenderAssetManifestReceipt Receipt() const { return receipt_; }
    [[nodiscard]] FarmRenderAssetManifestError LastError() const { return lastError_; }
    [[nodiscard]] bool IsBound() const { return bound_; }
private:
    bool Fail(FarmRenderAssetManifestError error) { lastError_ = error; return false; }
    FarmSpriteAssetSet assetSet_{};
    std::array<AssetResourceHandle, kRequiredAssetCount> leases_{};
    std::array<uint64_t, kRequiredAssetCount> expectedHashes_{};
    FarmRenderAssetManifestReceipt receipt_{};
    FarmRenderAssetManifestError lastError_ = FarmRenderAssetManifestError::None;
    bool bound_ = false;
};

} // namespace NeoEngine
