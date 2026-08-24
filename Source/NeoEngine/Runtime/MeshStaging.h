#pragma once

#include "AssetRegistry.h"
#include "ObjMeshImporter.h"

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {
enum class MeshStagingError : uint8_t { None, MissingAsset, WrongKind, AssetNotReady, DuplicateResource, ImportFailed, CapacityExceeded };
struct MeshStagingOptions { bool generateFlatNormals = false; };
struct CpuMeshResource { std::string assetId; uint64_t sourceHash = 0; bool generatedFlatNormals = false; std::vector<MeshVertex> vertices; std::vector<uint16_t> indices; };
class MeshStagingStore {
public:
    static constexpr size_t kMaxMeshes = 128U;
    static constexpr size_t kMaxStoredVertices = MeshRenderer::kMaxVertices;
    static constexpr size_t kMaxStoredIndices = MeshRenderer::kMaxIndices;
    bool StageObj(const AssetRegistry& registry,std::string_view assetId,const MeshStagingOptions& options = {});
    // Re-imports replacement registry bytes using the resource's original normal-generation option.
    bool Refresh(const AssetRegistry& registry,std::string_view assetId);
    // Validates replacement OBJ bytes and retained capacity without changing the staged resource.
    [[nodiscard]] bool CanRefresh(const AssetRegistry& registry,std::string_view assetId) const;
    [[nodiscard]] const CpuMeshResource* Find(std::string_view assetId) const;
    [[nodiscard]] bool IsCurrent(const AssetRegistry& registry,std::string_view assetId) const { const CpuMeshResource* resource=Find(assetId);const AssetDefinition* definition=registry.Find(assetId);return resource!=nullptr&&definition!=nullptr&&definition->kind==AssetKind::Mesh&&definition->state==AssetState::Ready&&definition->contentHash==resource->sourceHash; }
    [[nodiscard]] const std::deque<CpuMeshResource>& Resources() const { return resources_; }
    [[nodiscard]] size_t ResourceCount() const { return resources_.size(); }
    [[nodiscard]] size_t StagedVertices() const { return stagedVertices_; }
    [[nodiscard]] size_t StagedIndices() const { return stagedIndices_; }
    [[nodiscard]] MeshStagingError LastError() const { return lastError_; }
private:
    std::deque<CpuMeshResource> resources_;
    size_t stagedVertices_ = 0U;
    size_t stagedIndices_ = 0U;
    MeshStagingError lastError_ = MeshStagingError::None;
};
} // namespace NeoEngine
