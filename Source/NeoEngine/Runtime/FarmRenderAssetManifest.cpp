#include "Runtime/FarmRenderAssetManifest.h"

#include <new>
#include <string_view>

namespace NeoEngine {
namespace {
std::array<std::string_view, FarmRenderAssetManifest::kRequiredAssetCount> Required(const FarmSpriteAssetSet& set) { return {set.emptyTile, set.tilledTile, set.growingTile, set.harvestableTile, set.farmhouse, set.barn, set.silo, set.market, set.workshop, set.townHall, set.farmer, set.builder, set.merchant, set.questGiver, set.ranger, set.player}; }
uint64_t Mix(uint64_t hash, uint64_t value) { for (uint8_t index = 0U; index < 8U; ++index) { hash ^= static_cast<uint8_t>(value >> (index * 8U)); hash *= 1099511628211ULL; } return hash; }
}

bool FarmRenderAssetManifest::Bind(const FarmSpriteAssetSet& assetSet, const AssetRegistry& registry, AssetResourceManager& resources) {
    if (bound_) return Fail(FarmRenderAssetManifestError::AlreadyBound);
    const auto ids = Required(assetSet);
    std::array<AssetResourceHandle, kRequiredAssetCount> candidateLeases{};
    std::array<uint64_t, kRequiredAssetCount> candidateHashes{};
    uint16_t acquired = 0U;
    uint64_t aggregateHash = 1469598103934665603ULL;
    for (uint16_t index = 0U; index < kRequiredAssetCount; ++index) {
        const AssetDefinition* definition = ids[index].empty() ? nullptr : registry.Find(ids[index]);
        const std::vector<uint8_t>* bytes = definition == nullptr ? nullptr : registry.Data(ids[index]);
        if (ids[index].empty()) return Fail(FarmRenderAssetManifestError::InvalidAssetSet);
        if (definition == nullptr) return Fail(FarmRenderAssetManifestError::MissingAsset);
        if (definition->kind != AssetKind::Texture) return Fail(FarmRenderAssetManifestError::WrongKind);
        if (definition->state != AssetState::Ready) return Fail(FarmRenderAssetManifestError::NotReady);
        if (definition->contentHash == 0U || bytes == nullptr || bytes->size() < 2U) return Fail(FarmRenderAssetManifestError::InvalidData);
        candidateHashes[index] = definition->contentHash;
        aggregateHash = Mix(aggregateHash, candidateHashes[index]);
    }
    for (uint16_t index = 0U; index < kRequiredAssetCount; ++index) {
        if (!resources.Acquire(ids[index], candidateLeases[index])) { for (uint16_t released = 0U; released < acquired; ++released) resources.Release(candidateLeases[released]); return Fail(FarmRenderAssetManifestError::AcquireFailed); }
        ++acquired;
        AssetResourceReceipt resource{};
        if (!resources.Query(candidateLeases[index], resource) || resource.assetId != ids[index] || resource.state != AssetResourceState::Ready || resource.contentHash != candidateHashes[index]) { for (uint16_t released = 0U; released < acquired; ++released) resources.Release(candidateLeases[released]); return Fail(FarmRenderAssetManifestError::ResourceMismatch); }
    }
    try { assetSet_ = assetSet; } catch (const std::bad_alloc&) { for (uint16_t released = 0U; released < acquired; ++released) resources.Release(candidateLeases[released]); return Fail(FarmRenderAssetManifestError::Capacity); }
    leases_ = candidateLeases;
    expectedHashes_ = candidateHashes;
    receipt_ = {kRequiredAssetCount, aggregateHash};
    bound_ = true;
    lastError_ = FarmRenderAssetManifestError::None;
    return true;
}

bool FarmRenderAssetManifest::Validate(const AssetRegistry& registry, const AssetResourceManager& resources) {
    if (!bound_) return Fail(FarmRenderAssetManifestError::InvalidAssetSet);
    const auto ids = Required(assetSet_);
    uint64_t aggregateHash = 1469598103934665603ULL;
    for (uint16_t index = 0U; index < kRequiredAssetCount; ++index) {
        const AssetDefinition* definition = registry.Find(ids[index]);
        AssetResourceReceipt resource{};
        if (definition == nullptr) return Fail(FarmRenderAssetManifestError::MissingAsset);
        if (definition->kind != AssetKind::Texture) return Fail(FarmRenderAssetManifestError::WrongKind);
        if (definition->state != AssetState::Ready) return Fail(FarmRenderAssetManifestError::NotReady);
        if (definition->contentHash != expectedHashes_[index] || !resources.Query(leases_[index], resource) || resource.assetId != ids[index] || resource.state != AssetResourceState::Ready || resource.contentHash != expectedHashes_[index]) return Fail(FarmRenderAssetManifestError::ResourceMismatch);
        aggregateHash = Mix(aggregateHash, expectedHashes_[index]);
    }
    if (aggregateHash != receipt_.aggregateContentHash) return Fail(FarmRenderAssetManifestError::ResourceMismatch);
    lastError_ = FarmRenderAssetManifestError::None;
    return true;
}

} // namespace NeoEngine
