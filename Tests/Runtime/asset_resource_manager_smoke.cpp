#include "Runtime/AssetResourceManager.h"

#include <cstdint>
#include <vector>

int main() {
    using namespace NeoEngine;
    AssetRegistry registry;
    if (!registry.ImportBytes("texture.wheat", AssetKind::Texture, {}, {1U, 2U, 3U}) || !registry.ImportBytes("mesh.crop", AssetKind::Mesh, {"texture.wheat"}, {4U, 5U}) || !registry.ImportBytes("material.crop", AssetKind::Material, {"mesh.crop"}, {6U, 7U, 8U}) || !registry.MarkReady("texture.wheat") || !registry.MarkReady("mesh.crop") || !registry.MarkReady("material.crop")) return 1;
    AssetResourceManager resources(registry);

    AssetResourceHandle materialHandle{};
    if (!resources.Acquire("material.crop", materialHandle) || materialHandle.generation == 0U || resources.ActiveResourceCount() != 3U || resources.TotalLeaseCount() != 3U) return 2;
    AssetResourceReceipt materialReceipt{};
    if (!resources.Query(materialHandle, materialReceipt) || materialReceipt.assetId != "material.crop" || materialReceipt.refCount != 1U || materialReceipt.dependencyCount != 2U || resources.Data(materialHandle) == nullptr) return 3;
    AssetResourceReceipt textureReceipt{};
    if (!resources.Query("texture.wheat", textureReceipt) || textureReceipt.refCount != 1U) return 4;

    AssetResourceHandle textureHandle{};
    if (!resources.Acquire("texture.wheat", textureHandle) || resources.TotalLeaseCount() != 4U || textureHandle.slot == materialHandle.slot) return 5;
    if (!resources.Release(materialHandle) || resources.TotalLeaseCount() != 1U || resources.Query(materialHandle, materialReceipt) || resources.LastError() != AssetResourceError::InvalidHandle) return 6;
    if (!resources.Release(textureHandle) || resources.TotalLeaseCount() != 0U) return 7;

    if (!registry.ReplaceBytes("texture.wheat", {9U, 8U, 7U}) || !resources.SyncHotReload("texture.wheat") || resources.Query("texture.wheat", textureReceipt) == false || textureReceipt.state != AssetResourceState::Ready || textureReceipt.contentHash == 0U) return 8;
    AssetResourceHandle meshHandle{};
    if (!resources.Acquire("mesh.crop", meshHandle) || resources.ActiveResourceCount() != 3U) return 9;
    if (!registry.ReplaceBytes("texture.wheat", {10U, 11U}) || resources.SyncHotReload("texture.wheat") || resources.LastError() != AssetResourceError::StaleInUse) return 10;
    if (!resources.Query(meshHandle, materialReceipt) || materialReceipt.state != AssetResourceState::Ready) return 11;
    if (!resources.Release(meshHandle) || resources.TotalLeaseCount() != 0U || !resources.SyncHotReload("texture.wheat") || !resources.Query("texture.wheat", textureReceipt) || textureReceipt.state != AssetResourceState::Ready) return 12;

    AssetResourceHandle invalid{0U, 999999U};
    if (resources.Release(invalid) || resources.LastError() != AssetResourceError::InvalidHandle || resources.Data(invalid) != nullptr) return 13;
    if (resources.Acquire("missing.asset", materialHandle) || resources.LastError() != AssetResourceError::MissingDependency) return 14;
    if (!resources.ReloadIfSafe("never-loaded") || resources.LastError() != AssetResourceError::None) return 15;
    return 0;
}
