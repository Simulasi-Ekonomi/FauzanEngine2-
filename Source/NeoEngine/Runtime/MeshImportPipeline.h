#pragma once

#include "MeshStaging.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
enum class MeshImportPipelineError : uint8_t { None, InvalidRequest, BatchCapacity, RegistryImportFailed, RegistryReadyFailed, StageFailed };
struct MeshImportReceipt { std::string assetId; uint64_t contentHash = 0; uint16_t vertexCount = 0; uint16_t indexCount = 0; bool generatedFlatNormals = false; };
struct MeshImportRequest { std::string assetId; std::vector<std::string> dependencies; std::vector<uint8_t> bytes; MeshStagingOptions options{}; };

// Candidate-only OBJ import transaction. It does not read files, parse MTL, upload GPU
// resources, or refresh a live scene adapter.
class MeshImportPipeline {
public:
    static constexpr size_t kMaxImportSetEntries = 32U;
    bool ImportObj(AssetRegistry& registry, MeshStagingStore& meshes, std::string assetId, std::vector<std::string> dependencies, std::vector<uint8_t> bytes, MeshStagingOptions options, MeshImportReceipt& receipt);
    bool ImportObjSet(AssetRegistry& registry, MeshStagingStore& meshes, const std::vector<MeshImportRequest>& requests, std::vector<MeshImportReceipt>& receipts);
    [[nodiscard]] MeshImportPipelineError LastError() const { return lastError_; }
private:
    MeshImportPipelineError lastError_ = MeshImportPipelineError::None;
};
} // namespace NeoEngine
