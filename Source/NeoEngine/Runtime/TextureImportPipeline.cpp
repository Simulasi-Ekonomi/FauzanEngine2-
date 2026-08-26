#include "Runtime/TextureImportPipeline.h"

namespace NeoEngine {
bool TextureImportPipeline::Import(AssetRegistry& registry, TextureStagingStore& textures, std::string assetId, std::vector<std::string> dependencies, std::vector<uint8_t> bytes, TextureImportFormat format, TextureImportReceipt& receipt) {
    if (assetId.empty() || bytes.empty()) { lastError_ = TextureImportPipelineError::InvalidRequest; return false; }
    AssetRegistry candidateRegistry = registry; TextureStagingStore candidateTextures = textures;
    if (!candidateRegistry.ImportBytes(assetId, AssetKind::Texture, std::move(dependencies), std::move(bytes))) { lastError_ = TextureImportPipelineError::RegistryImportFailed; return false; }
    if (!candidateRegistry.MarkReady(assetId)) { lastError_ = TextureImportPipelineError::RegistryReadyFailed; return false; }
    const bool staged = format == TextureImportFormat::PpmP6 ? candidateTextures.StagePpm(candidateRegistry, assetId) : candidateTextures.StageBmp(candidateRegistry, assetId);
    if (!staged) { lastError_ = TextureImportPipelineError::StageFailed; return false; }
    const AssetDefinition* definition = candidateRegistry.Find(assetId); const CpuTextureResource* resource = candidateTextures.Find(assetId);
    if (definition == nullptr || resource == nullptr) { lastError_ = TextureImportPipelineError::StageFailed; return false; }
    TextureImportReceipt candidateReceipt{assetId, definition->contentHash, resource->width, resource->height, format}; registry = std::move(candidateRegistry); textures = std::move(candidateTextures); receipt = std::move(candidateReceipt); lastError_ = TextureImportPipelineError::None; return true;
}
bool TextureImportPipeline::ImportSet(AssetRegistry& registry, TextureStagingStore& textures, const std::vector<TextureImportRequest>& requests, std::vector<TextureImportReceipt>& receipts) {
    if (requests.empty()) { lastError_ = TextureImportPipelineError::InvalidRequest; return false; }
    if (requests.size() > kMaxImportSetEntries) { lastError_ = TextureImportPipelineError::BatchCapacity; return false; }
    AssetRegistry candidateRegistry = registry; TextureStagingStore candidateTextures = textures; std::vector<TextureImportReceipt> candidateReceipts; candidateReceipts.reserve(requests.size());
    for (const TextureImportRequest& request : requests) { TextureImportReceipt receipt{}; if (!Import(candidateRegistry, candidateTextures, request.assetId, request.dependencies, request.bytes, request.format, receipt)) return false; candidateReceipts.push_back(std::move(receipt)); }
    registry = std::move(candidateRegistry); textures = std::move(candidateTextures); receipts = std::move(candidateReceipts); lastError_ = TextureImportPipelineError::None; return true;
}
} // namespace NeoEngine
