#pragma once

#include "AssetRegistry.h"
#include "BmpTexture.h"
#include "PpmTexture.h"

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {

enum class TextureStagingError : uint8_t { None, MissingAsset, WrongKind, AssetNotReady, DuplicateResource, DecodeFailed, CapacityExceeded };
enum class TextureSourceFormat : uint8_t { PpmP6, BmpBiRgb };

struct CpuTextureResource {
    std::string assetId;
    uint64_t sourceHash = 0;
    TextureSourceFormat sourceFormat = TextureSourceFormat::PpmP6;
    uint16_t width = 0;
    uint16_t height = 0;
    std::vector<uint8_t> rgba;
};

class TextureStagingStore {
public:
    static constexpr size_t kMaxTextures = 512;
    static constexpr size_t kMaxDecodedBytes = 32U * 1024U * 1024U;

    bool StagePpm(const AssetRegistry& registry, std::string_view assetId);
    bool StageBmp(const AssetRegistry& registry, std::string_view assetId);
    // Re-decodes replacement registry bytes into an existing resource without changing its address.
    bool Refresh(const AssetRegistry& registry, std::string_view assetId);
    // Validates replacement bytes and retained capacity without changing the staged resource.
    [[nodiscard]] bool CanRefresh(const AssetRegistry& registry, std::string_view assetId) const;
    [[nodiscard]] const CpuTextureResource* Find(std::string_view assetId) const;
    [[nodiscard]] bool IsCurrent(const AssetRegistry& registry,std::string_view assetId) const { const CpuTextureResource* resource=Find(assetId);const AssetDefinition* definition=registry.Find(assetId);return resource!=nullptr&&definition!=nullptr&&definition->kind==AssetKind::Texture&&definition->state==AssetState::Ready&&definition->contentHash==resource->sourceHash; }
    [[nodiscard]] const std::deque<CpuTextureResource>& Resources() const { return resources_; }
    [[nodiscard]] size_t ResourceCount() const { return resources_.size(); }
    [[nodiscard]] size_t DecodedBytes() const { return decodedBytes_; }
    [[nodiscard]] TextureStagingError LastError() const { return lastError_; }

private:
    std::deque<CpuTextureResource> resources_;
    size_t decodedBytes_ = 0;
    TextureStagingError lastError_ = TextureStagingError::None;
};

} // namespace NeoEngine
