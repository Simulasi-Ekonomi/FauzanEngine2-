#pragma once

#include "AssetRegistry.h"
#include "MtlMaterialImporter.h"

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>

namespace NeoEngine {
enum class MaterialStagingError : uint8_t { None, MissingAsset, WrongKind, AssetNotReady, DuplicateResource, ImportFailed, CapacityExceeded };
struct CpuMaterialResource { std::string assetId; std::string materialName; uint64_t sourceHash = 0U; MeshMaterial material{}; };
class MaterialStagingStore {
public:
    static constexpr size_t kMaxMaterials = 128U;
    bool StageMtl(const AssetRegistry& registry,std::string_view assetId,std::string_view materialName);
    // Re-imports replacement registry bytes into the named material resource in place.
    bool Refresh(const AssetRegistry& registry,std::string_view assetId,std::string_view materialName);
    // Validates replacement MTL bytes for the retained named material without mutation.
    [[nodiscard]] bool CanRefresh(const AssetRegistry& registry,std::string_view assetId,std::string_view materialName) const;
    [[nodiscard]] const CpuMaterialResource* Find(std::string_view assetId,std::string_view materialName) const;
    [[nodiscard]] bool IsCurrent(const AssetRegistry& registry,std::string_view assetId,std::string_view materialName) const { const CpuMaterialResource* resource=Find(assetId,materialName);const AssetDefinition* definition=registry.Find(assetId);return resource!=nullptr&&definition!=nullptr&&definition->kind==AssetKind::Material&&definition->state==AssetState::Ready&&definition->contentHash==resource->sourceHash; }
    [[nodiscard]] const std::deque<CpuMaterialResource>& Resources() const { return resources_; }
    [[nodiscard]] size_t ResourceCount() const { return resources_.size(); }
    [[nodiscard]] MaterialStagingError LastError() const { return lastError_; }
private:
    std::deque<CpuMaterialResource> resources_;
    MaterialStagingError lastError_ = MaterialStagingError::None;
};
} // namespace NeoEngine
