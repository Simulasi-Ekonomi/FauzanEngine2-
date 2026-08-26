#include "Runtime/AssetResourceManager.h"

#include <cstdint>
#include <vector>

int main() {
    using namespace NeoEngine;
    AssetRegistry registry;
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

    AssetResourceHandle invalid{0U, 999999U};
    if (resources.Release(invalid) || resources.LastError() != AssetResourceError::InvalidHandle || resources.Data(invalid) != nullptr) return 15;
    if (resources.Acquire("missing.asset", materialHandle) || resources.LastError() != AssetResourceError::MissingDependency) return 16;
    if (!resources.ReloadIfSafe("never-loaded") || resources.LastError() != AssetResourceError::None) return 17;
    uint16_t evictedResources = 0U;
    if (!resources.EvictUnleased(evictedResources) || evictedResources != 3U || resources.ActiveResourceCount() != 0U || resources.Query("texture.wheat", textureReceipt) || resources.LastError() != AssetResourceError::InvalidIdentifier) return 18;
    AssetResourceHandle rehydratedHandle{};
    if (!resources.Acquire("material.crop", rehydratedHandle) || resources.ActiveResourceCount() != 3U || resources.ActiveLeaseCount() != 1U || !resources.Release(rehydratedHandle) || resources.ActiveLeaseCount() != 0U) return 19;
    return 0;
}
