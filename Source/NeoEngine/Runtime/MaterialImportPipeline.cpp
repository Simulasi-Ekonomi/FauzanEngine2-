#include "Runtime/MaterialImportPipeline.h"

#include <algorithm>

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
bool MaterialImportPipeline::ImportMtlSet(AssetRegistry& registry, MaterialStagingStore& materials, std::string assetId, std::vector<std::string> dependencies, std::vector<uint8_t> bytes, std::vector<std::string> materialNames, std::vector<MaterialImportReceipt>& receipts) {
    if (assetId.empty() || bytes.empty() || materialNames.empty() || materialNames.size() > kMaxMaterialSet) { lastError_ = MaterialImportPipelineError::InvalidRequest; return false; }
    for (size_t index = 0U; index < materialNames.size(); ++index) if (materialNames[index].empty() || std::find(materialNames.begin(), materialNames.begin() + static_cast<std::ptrdiff_t>(index), materialNames[index]) != materialNames.begin() + static_cast<std::ptrdiff_t>(index)) { lastError_ = MaterialImportPipelineError::InvalidRequest; return false; }
    AssetRegistry candidateRegistry = registry; MaterialStagingStore candidateMaterials = materials;
    if (!candidateRegistry.ImportBytes(assetId, AssetKind::Material, std::move(dependencies), std::move(bytes))) { lastError_ = MaterialImportPipelineError::RegistryImportFailed; return false; }
    if (!candidateRegistry.MarkReady(assetId)) { lastError_ = MaterialImportPipelineError::RegistryReadyFailed; return false; }
    std::vector<MaterialImportReceipt> candidateReceipts; candidateReceipts.reserve(materialNames.size()); const AssetDefinition* definition = candidateRegistry.Find(assetId);
    if (definition == nullptr) { lastError_ = MaterialImportPipelineError::StageFailed; return false; }
    for (const std::string& materialName : materialNames) { if (!candidateMaterials.StageMtl(candidateRegistry, assetId, materialName)) { lastError_ = MaterialImportPipelineError::StageFailed; return false; } const CpuMaterialResource* resource = candidateMaterials.Find(assetId, materialName); if (resource == nullptr) { lastError_ = MaterialImportPipelineError::StageFailed; return false; } candidateReceipts.push_back({assetId, materialName, definition->contentHash, resource->material.rgba}); }
    registry = std::move(candidateRegistry); materials = std::move(candidateMaterials); receipts = std::move(candidateReceipts); lastError_ = MaterialImportPipelineError::None; return true;
}
bool MaterialImportPipeline::RefreshMtl(AssetRegistry& registry, MaterialStagingStore& materials, std::string assetId, std::vector<uint8_t> bytes, std::string materialName, MaterialImportReceipt& receipt) {
    if (assetId.empty() || materialName.empty() || bytes.empty()) { lastError_ = MaterialImportPipelineError::InvalidRequest; return false; }
    AssetRegistry candidateRegistry = registry; MaterialStagingStore candidateMaterials = materials;
    if (!candidateRegistry.ReplaceBytes(assetId, std::move(bytes))) { lastError_ = MaterialImportPipelineError::RegistryReplaceFailed; return false; }
    if (!candidateMaterials.Refresh(candidateRegistry, assetId, materialName)) { lastError_ = MaterialImportPipelineError::StageFailed; return false; }
    const AssetDefinition* definition = candidateRegistry.Find(assetId); const CpuMaterialResource* resource = candidateMaterials.Find(assetId, materialName);
    if (definition == nullptr || resource == nullptr) { lastError_ = MaterialImportPipelineError::StageFailed; return false; }
    MaterialImportReceipt candidateReceipt{assetId, materialName, definition->contentHash, resource->material.rgba};
    registry = std::move(candidateRegistry); materials = std::move(candidateMaterials); receipt = std::move(candidateReceipt); lastError_ = MaterialImportPipelineError::None; return true;
}
bool MaterialImportPipeline::RefreshMtlSet(AssetRegistry& registry, MaterialStagingStore& materials, std::string assetId, std::vector<uint8_t> bytes, std::vector<std::string> materialNames, std::vector<MaterialImportReceipt>& receipts) {
    if (assetId.empty() || bytes.empty() || materialNames.empty() || materialNames.size() > kMaxMaterialSet) { lastError_ = MaterialImportPipelineError::InvalidRequest; return false; }
    for (size_t index = 0U; index < materialNames.size(); ++index) if (materialNames[index].empty() || std::find(materialNames.begin(), materialNames.begin() + static_cast<std::ptrdiff_t>(index), materialNames[index]) != materialNames.begin() + static_cast<std::ptrdiff_t>(index)) { lastError_ = MaterialImportPipelineError::InvalidRequest; return false; }
    AssetRegistry candidateRegistry = registry; MaterialStagingStore candidateMaterials = materials;
    if (!candidateRegistry.ReplaceBytes(assetId, std::move(bytes))) { lastError_ = MaterialImportPipelineError::RegistryReplaceFailed; return false; }
    const AssetDefinition* definition = candidateRegistry.Find(assetId); if (definition == nullptr) { lastError_ = MaterialImportPipelineError::StageFailed; return false; }
    std::vector<MaterialImportReceipt> candidateReceipts; candidateReceipts.reserve(materialNames.size());
    for (const std::string& materialName : materialNames) { if (!candidateMaterials.Refresh(candidateRegistry, assetId, materialName)) { lastError_ = MaterialImportPipelineError::StageFailed; return false; } const CpuMaterialResource* resource = candidateMaterials.Find(assetId, materialName); if (resource == nullptr) { lastError_ = MaterialImportPipelineError::StageFailed; return false; } candidateReceipts.push_back({assetId, materialName, definition->contentHash, resource->material.rgba}); }
    registry = std::move(candidateRegistry); materials = std::move(candidateMaterials); receipts = std::move(candidateReceipts); lastError_ = MaterialImportPipelineError::None; return true;
}
} // namespace NeoEngine
