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
} // namespace NeoEngine
