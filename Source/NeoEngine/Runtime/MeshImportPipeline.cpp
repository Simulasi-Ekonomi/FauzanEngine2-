#include "Runtime/MeshImportPipeline.h"

namespace NeoEngine {
bool MeshImportPipeline::ImportObj(AssetRegistry& registry, MeshStagingStore& meshes, std::string assetId, std::vector<std::string> dependencies, std::vector<uint8_t> bytes, MeshStagingOptions options, MeshImportReceipt& receipt) {
    if (assetId.empty() || bytes.empty()) { lastError_ = MeshImportPipelineError::InvalidRequest; return false; }
    AssetRegistry candidateRegistry = registry; MeshStagingStore candidateMeshes = meshes;
    if (!candidateRegistry.ImportBytes(assetId, AssetKind::Mesh, std::move(dependencies), std::move(bytes))) { lastError_ = MeshImportPipelineError::RegistryImportFailed; return false; }
    if (!candidateRegistry.MarkReady(assetId)) { lastError_ = MeshImportPipelineError::RegistryReadyFailed; return false; }
    if (!candidateMeshes.StageObj(candidateRegistry, assetId, options)) { lastError_ = MeshImportPipelineError::StageFailed; return false; }
    const AssetDefinition* definition = candidateRegistry.Find(assetId); const CpuMeshResource* resource = candidateMeshes.Find(assetId);
    if (definition == nullptr || resource == nullptr || resource->vertices.size() > UINT16_MAX || resource->indices.size() > UINT16_MAX) { lastError_ = MeshImportPipelineError::StageFailed; return false; }
    MeshImportReceipt candidateReceipt{assetId, definition->contentHash, static_cast<uint16_t>(resource->vertices.size()), static_cast<uint16_t>(resource->indices.size()), resource->generatedFlatNormals}; registry = std::move(candidateRegistry); meshes = std::move(candidateMeshes); receipt = std::move(candidateReceipt); lastError_ = MeshImportPipelineError::None; return true;
}
} // namespace NeoEngine
