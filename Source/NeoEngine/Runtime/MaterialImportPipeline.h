#pragma once

#include "MaterialStaging.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
enum class MaterialImportPipelineError : uint8_t { None, InvalidRequest, RegistryImportFailed, RegistryReadyFailed, RegistryReplaceFailed, StageFailed };
struct MaterialImportReceipt { std::string assetId; std::string materialName; uint64_t contentHash = 0U; uint32_t rgba = 0xFFFFFFFFU; };

// Candidate-only in-memory MTL transaction. It does not read paths, resolve mtllib,
// watch files, refresh live scene bindings, upload GPU resources, or persist bytes.
class MaterialImportPipeline {
public:
    bool ImportMtl(AssetRegistry& registry, MaterialStagingStore& materials, std::string assetId, std::vector<std::string> dependencies, std::vector<uint8_t> bytes, std::string materialName, MaterialImportReceipt& receipt);
    bool RefreshMtl(AssetRegistry& registry, MaterialStagingStore& materials, std::string assetId, std::vector<uint8_t> bytes, std::string materialName, MaterialImportReceipt& receipt);
    [[nodiscard]] MaterialImportPipelineError LastError() const { return lastError_; }
private:
    MaterialImportPipelineError lastError_ = MaterialImportPipelineError::None;
};
} // namespace NeoEngine
