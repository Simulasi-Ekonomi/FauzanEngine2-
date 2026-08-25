#pragma once

#include "TextureStaging.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
enum class TextureImportPipelineError : uint8_t { None, InvalidRequest, RegistryImportFailed, RegistryReadyFailed, StageFailed };
enum class TextureImportFormat : uint8_t { PpmP6, BmpBiRgb };
struct TextureImportReceipt { std::string assetId; uint64_t contentHash = 0; uint16_t width = 0; uint16_t height = 0; TextureImportFormat format = TextureImportFormat::PpmP6; };

// Candidate-only in-memory import transaction. It does not read files, watch paths,
// upload GPU resources, or refresh a live renderer adapter.
class TextureImportPipeline {
public:
    bool Import(AssetRegistry& registry, TextureStagingStore& textures, std::string assetId, std::vector<std::string> dependencies, std::vector<uint8_t> bytes, TextureImportFormat format, TextureImportReceipt& receipt);
    [[nodiscard]] TextureImportPipelineError LastError() const { return lastError_; }
private:
    TextureImportPipelineError lastError_ = TextureImportPipelineError::None;
};
} // namespace NeoEngine
