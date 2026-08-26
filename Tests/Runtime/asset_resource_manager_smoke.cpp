#include "Runtime/AssetResourceManager.h"

#include <cstdint>
#include <string>
#include <vector>

int main() {
    using namespace NeoEngine;
    AssetRegistry registry;
    if (registry.Declare(std::string("nul\0asset", 9U), AssetKind::Texture, {}) || registry.LastError() != AssetRegistryError::InvalidIdentifier || !registry.All().empty()) return 1;
    if (!registry.ImportBytes("texture.wheat", AssetKind::Texture, {}, {1U, 2U, 3U}) || !registry.ImportBytes("mesh.crop", AssetKind::Mesh, {"texture.wheat"}, {4U, 5U}) || !registry.ImportBytes("material.crop", AssetKind::Material, {"mesh.crop"}, {6U, 7U, 8U}) || !registry.MarkReady("texture.wheat") || !registry.MarkReady("mesh.crop") || !registry.MarkReady("material.crop")) return 1;
    AssetResourceManager resources(registry);

    AssetResourceHandle materialHandle{};
    if (!resources.Acquire("material.crop", materialHandle) || materialHandle.generation == 0U || resources.ActiveResourceCount() != 3U || resources.TotalLeaseCount() != 3U || resources.ActiveLeaseCount() != 1U) return 2;
    AssetResourceReceipt materialReceipt{};
    if (!resources.Query(materialHandle, materialReceipt) || materialReceipt.assetId != "material.crop" || materialReceipt.refCount != 1U || materialReceipt.dependencyCount != 2U || materialReceipt.resourceGeneration == 0U || resources.Data(materialHandle) == nullptr) return 3;
    AssetResourceHandle materialHandle2{};
    if (!resources.Acquire("material.crop", materialHandle2) || materialHandle2.slot == materialHandle.slot || materialHandle2.generation == 0U || resources.TotalLeaseCount() != 6U || resources.ActiveLeaseCount() != 2U || !resources.Query(materialHandle, materialReceipt) || materialReceipt.refCount != 2U) return 4;
    AssetResourceReceipt textureReceipt{};
    if (!resources.Query("texture.wheat", textureReceipt) || textureReceipt.refCount != 2U) return 5;

    if (!resources.Release(materialHandle2) || resources.TotalLeaseCount() != 3U || resources.ActiveLeaseCount() != 1U || resources.Query(materialHandle2, materialReceipt) || resources.LastError() != AssetResourceError::InvalidHandle || !resources.Query(materialHandle, materialReceipt) || materialReceipt.refCount != 1U) return 6;
    AssetResourceHandle textureHandle{};
    if (!resources.Acquire("texture.wheat", textureHandle) || resources.TotalLeaseCount() != 4U || resources.ActiveLeaseCount() != 2U || textureHandle.slot == materialHandle.slot) return 7;
    if (!resources.Release(materialHandle) || resources.TotalLeaseCount() != 1U || resources.ActiveLeaseCount() != 1U || resources.Query(materialHandle, materialReceipt) || resources.LastError() != AssetResourceError::InvalidHandle) return 8;
    if (!resources.Release(textureHandle) || resources.TotalLeaseCount() != 0U || resources.ActiveLeaseCount() != 0U) return 9;

    const uint32_t textureGenerationBeforeReload = textureReceipt.resourceGeneration;
    if (!registry.ReplaceBytes("texture.wheat", {9U, 8U, 7U}) || !resources.SyncHotReload("texture.wheat") || resources.Query("texture.wheat", textureReceipt) == false || textureReceipt.state != AssetResourceState::Ready || textureReceipt.contentHash == 0U || textureReceipt.resourceGeneration <= textureGenerationBeforeReload) return 10;
    AssetResourceHandle meshHandle{};
    if (!resources.Acquire("mesh.crop", meshHandle) || resources.ActiveResourceCount() != 3U || resources.ActiveLeaseCount() != 1U) return 11;
    if (!registry.ReplaceBytes("texture.wheat", {10U, 11U}) || resources.SyncHotReload("texture.wheat") || resources.LastError() != AssetResourceError::StaleInUse) return 12;
    if (!resources.Query(meshHandle, materialReceipt) || materialReceipt.state != AssetResourceState::Ready) return 13;
    if (!resources.Release(meshHandle) || resources.TotalLeaseCount() != 0U || resources.ActiveLeaseCount() != 0U || !resources.SyncHotReload("texture.wheat") || !resources.Query("texture.wheat", textureReceipt) || textureReceipt.state != AssetResourceState::Ready) return 14;
    const uint64_t textureHashBeforeInvalidReplace = textureReceipt.contentHash;
    if (registry.ReplaceBytes("bad id", {12U}) || registry.LastError() != AssetRegistryError::InvalidIdentifier || !resources.Query("texture.wheat", textureReceipt) || textureReceipt.contentHash != textureHashBeforeInvalidReplace) return 14;

    AssetResourceHandle invalid{0U, 999999U};
    materialReceipt.assetId = "query-preserve";
    if (resources.Release(invalid) || resources.LastError() != AssetResourceError::InvalidHandle || resources.Data(invalid) != nullptr || resources.Query(invalid, materialReceipt) || resources.LastError() != AssetResourceError::InvalidHandle || materialReceipt.assetId != "query-preserve") return 15;
    const AssetResourceHandle preservedAcquireHandle{123U, 456U};
    materialHandle = preservedAcquireHandle;
    if (resources.Acquire("missing.asset", materialHandle) || resources.LastError() != AssetResourceError::MissingDependency || materialHandle != preservedAcquireHandle) return 16;
    if (!registry.Declare("declared.asset", AssetKind::Audio, {})) return 16;
    const uint16_t resourcesBeforeNotReady = resources.ActiveResourceCount();
    materialHandle = preservedAcquireHandle;
    if (resources.Acquire("declared.asset", materialHandle) || resources.LastError() != AssetResourceError::NotReady || resources.ActiveResourceCount() != resourcesBeforeNotReady || resources.ActiveLeaseCount() != 0U || materialHandle != preservedAcquireHandle) return 16;
    AssetRegistry depthRegistry;
    if (!depthRegistry.ImportBytes("depth0", AssetKind::Prefab, {}, {1U}) || !depthRegistry.MarkReady("depth0")) return 16;
    for (uint8_t depth = 1U; depth <= 16U; ++depth) {
        const std::string id = "depth" + std::to_string(depth);
        const std::string dependency = "depth" + std::to_string(static_cast<uint8_t>(depth - 1U));
        if (!depthRegistry.ImportBytes(id, AssetKind::Prefab, {dependency}, {1U}) || !depthRegistry.MarkReady(id)) return 16;
    }
    AssetResourceManager depthResources(depthRegistry);
    const uint16_t depthResourcesBefore = depthResources.ActiveResourceCount();
    if (depthResources.Acquire("depth16", materialHandle) || depthResources.LastError() != AssetResourceError::Capacity || depthResources.ActiveResourceCount() != depthResourcesBefore || depthResources.ActiveLeaseCount() != 0U) return 16;
    if (!resources.ReloadIfSafe("never-loaded") || resources.LastError() != AssetResourceError::None) return 17;
    uint16_t evictedResources = 0U;
    if (!resources.EvictUnleased(evictedResources) || evictedResources != 3U || resources.ActiveResourceCount() != 0U || resources.Query("texture.wheat", textureReceipt) || resources.LastError() != AssetResourceError::InvalidIdentifier) return 18;
    AssetResourceHandle rehydratedHandle{};
    if (!resources.Acquire("material.crop", rehydratedHandle) || resources.ActiveResourceCount() != 3U || resources.ActiveLeaseCount() != 1U || resources.ResidentBytes() != 7U) return 19;
    uint32_t budgetResidentBytes = 999U;
    uint16_t budgetEvictedResources = 99U;
    if (resources.EvictToBudget(0U, budgetResidentBytes, budgetEvictedResources) || resources.LastError() != AssetResourceError::BudgetExceeded || budgetResidentBytes != 999U || budgetEvictedResources != 99U || resources.ActiveResourceCount() != 3U) return 20;
    if (!resources.Release(rehydratedHandle) || resources.ActiveLeaseCount() != 0U || !resources.EvictToBudget(2U, budgetResidentBytes, budgetEvictedResources) || budgetResidentBytes != 2U || resources.ResidentBytes() != budgetResidentBytes || resources.ActiveResourceCount() != 1U || budgetEvictedResources != 2U) return 21;
    if (!resources.EvictToBudget(0U, budgetResidentBytes, budgetEvictedResources) || budgetResidentBytes != 0U || resources.ResidentBytes() != 0U || resources.ActiveResourceCount() != 0U || budgetEvictedResources != 1U) return 22;
    return 0;
}
