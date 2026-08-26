#pragma once

#include "TextureStaging.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
enum class TextureImportPipelineError : uint8_t { None, InvalidRequest, BatchCapacity, RegistryImportFailed, RegistryReadyFailed, StageFailed };
enum class TextureImportFormat : uint8_t { PpmP6, BmpBiRgb };
struct TextureImportReceipt { std::string assetId; uint64_t contentHash = 0; uint16_t width = 0; uint16_t height = 0; TextureImportFormat format = TextureImportFormat::PpmP6; };
struct TextureImportRequest { std::string assetId; std::vector<std::string> dependencies; std::vector<uint8_t> bytes; TextureImportFormat format = TextureImportFormat::PpmP6; };

// Candidate-only in-memory import transaction. It does not read files, watch paths,
// upload GPU resources, or refresh a live renderer adapter.
class TextureImportPipeline {
public:
    static constexpr size_t kMaxImportSetEntries = 32U;
    bool Import(AssetRegistry& registry, TextureStagingStore& textures, std::string assetId, std::vector<std::string> dependencies, std::vector<uint8_t> bytes, TextureImportFormat format, TextureImportReceipt& receipt);
    bool ImportSet(AssetRegistry& registry, TextureStagingStore& textures, const std::vector<TextureImportRequest>& requests, std::vector<TextureImportReceipt>& receipts);
    [[nodiscard]] TextureImportPipelineError LastError() const { return lastError_; }
private:
    TextureImportPipelineError lastError_ = TextureImportPipelineError::None;
};
} // namespace NeoEngine
