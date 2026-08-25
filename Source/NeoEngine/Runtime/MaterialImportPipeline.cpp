#include "Runtime/MaterialImportPipeline.h"

namespace NeoEngine {
bool MaterialImportPipeline::ImportMtl(AssetRegistry& registry, MaterialStagingStore& materials, std::string assetId, std::vector<std::string> dependencies, std::vector<uint8_t> bytes, std::string materialName, MaterialImportReceipt& receipt) {
    if (assetId.empty() || materialName.empty() || bytes.empty()) { lastError_ = MaterialImportPipelineError::InvalidRequest; return false; }
    AssetRegistry candidateRegistry = registry; MaterialStagingStore candidateMaterials = materials;
    if (!candidateRegistry.ImportBytes(assetId, AssetKind::Material, std::move(dependencies), std::move(bytes))) { lastError_ = MaterialImportPipelineError::RegistryImportFailed; return false; }
    if (!candidateRegistry.MarkReady(assetId)) { lastError_ = MaterialImportPipelineError::RegistryReadyFailed; return false; }
    if (!candidateMaterials.StageMtl(candidateRegistry, assetId, materialName)) { lastError_ = MaterialImportPipelineError::StageFailed; return false; }
    const AssetDefinition* definition = candidateRegistry.Find(assetId); const CpuMaterialResource* resource = candidateMaterials.Find(assetId, materialName);
    if (definition == nullptr || resource == nullptr) { lastError_ = MaterialImportPipelineError::StageFailed; return false; }
    MaterialImportReceipt candidateReceipt{assetId, materialName, definition->contentHash, resource->material.rgba};
    registry = std::move(candidateRegistry); materials = std::move(candidateMaterials); receipt = std::move(candidateReceipt); lastError_ = MaterialImportPipelineError::None; return true;
}
} // namespace NeoEngine
