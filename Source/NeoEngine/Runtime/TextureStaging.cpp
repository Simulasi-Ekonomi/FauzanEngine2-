#include "TextureStaging.h"

#include <algorithm>

namespace NeoEngine {

bool TextureStagingStore::StagePpm(const AssetRegistry& registry, std::string_view assetId) {
    const AssetDefinition* definition = registry.Find(assetId);
    const std::vector<uint8_t>* bytes = registry.Data(assetId);
    if (definition == nullptr || bytes == nullptr) {
        lastError_ = TextureStagingError::MissingAsset;
        return false;
    }
    if (definition->kind != AssetKind::Texture) {
        lastError_ = TextureStagingError::WrongKind;
        return false;
    }
    if (definition->state != AssetState::Ready) {
        lastError_ = TextureStagingError::AssetNotReady;
        return false;
    }
    if (Find(assetId) != nullptr) {
        lastError_ = TextureStagingError::DuplicateResource;
        return false;
    }
    if (resources_.size() >= kMaxTextures) {
        lastError_ = TextureStagingError::CapacityExceeded;
        return false;
    }
    RgbaTexture decoded;
    TextureDecodeError decodeError = TextureDecodeError::None;
    if (!PpmTextureDecoder::DecodeP6(*bytes, decoded, decodeError)) {
        lastError_ = TextureStagingError::DecodeFailed;
        return false;
    }
    if (decoded.rgba.size() > kMaxDecodedBytes - decodedBytes_) {
        lastError_ = TextureStagingError::CapacityExceeded;
        return false;
    }
    decodedBytes_ += decoded.rgba.size();
    resources_.push_back({std::string(assetId), definition->contentHash, TextureSourceFormat::PpmP6, decoded.width, decoded.height, std::move(decoded.rgba)});
    lastError_ = TextureStagingError::None;
    return true;
}

bool TextureStagingStore::StageBmp(const AssetRegistry& registry, std::string_view assetId) {
    const AssetDefinition* definition = registry.Find(assetId);
    const std::vector<uint8_t>* bytes = registry.Data(assetId);
    if (definition == nullptr || bytes == nullptr) { lastError_ = TextureStagingError::MissingAsset; return false; }
    if (definition->kind != AssetKind::Texture) { lastError_ = TextureStagingError::WrongKind; return false; }
    if (definition->state != AssetState::Ready) { lastError_ = TextureStagingError::AssetNotReady; return false; }
    if (Find(assetId) != nullptr) { lastError_ = TextureStagingError::DuplicateResource; return false; }
    if (resources_.size() >= kMaxTextures) { lastError_ = TextureStagingError::CapacityExceeded; return false; }
    RgbaTexture decoded;TextureDecodeError decodeError=TextureDecodeError::None;
    if (!BmpTextureDecoder::DecodeBiRgb(*bytes,decoded,decodeError)) { lastError_ = TextureStagingError::DecodeFailed; return false; }
    if (decoded.rgba.size() > kMaxDecodedBytes-decodedBytes_) { lastError_ = TextureStagingError::CapacityExceeded; return false; }
    decodedBytes_ += decoded.rgba.size();resources_.push_back({std::string(assetId),definition->contentHash,TextureSourceFormat::BmpBiRgb,decoded.width,decoded.height,std::move(decoded.rgba)});lastError_=TextureStagingError::None;return true;
}

bool TextureStagingStore::Refresh(const AssetRegistry& registry, std::string_view assetId) {
    const AssetDefinition* definition = registry.Find(assetId);
    const std::vector<uint8_t>* bytes = registry.Data(assetId);
    if (definition == nullptr || bytes == nullptr) { lastError_ = TextureStagingError::MissingAsset; return false; }
    if (definition->kind != AssetKind::Texture) { lastError_ = TextureStagingError::WrongKind; return false; }
    if (definition->state != AssetState::Ready) { lastError_ = TextureStagingError::AssetNotReady; return false; }

    const auto found = std::find_if(resources_.begin(), resources_.end(), [assetId](const CpuTextureResource& value) { return value.assetId == assetId; });
    if (found == resources_.end()) { lastError_ = TextureStagingError::MissingAsset; return false; }

    RgbaTexture decoded;
    TextureDecodeError decodeError = TextureDecodeError::None;
    bool decodedOk = false;
    if (found->sourceFormat == TextureSourceFormat::PpmP6) {
        decodedOk = PpmTextureDecoder::DecodeP6(*bytes, decoded, decodeError);
    } else if (found->sourceFormat == TextureSourceFormat::BmpBiRgb) {
        decodedOk = BmpTextureDecoder::DecodeBiRgb(*bytes, decoded, decodeError);
    }
    if (!decodedOk) { lastError_ = TextureStagingError::DecodeFailed; return false; }

    const size_t retainedBytes = decodedBytes_ - found->rgba.size();
    if (decoded.rgba.size() > kMaxDecodedBytes - retainedBytes) { lastError_ = TextureStagingError::CapacityExceeded; return false; }

    const size_t replacementBytes = decoded.rgba.size();
    found->sourceHash = definition->contentHash;
    found->width = decoded.width;
    found->height = decoded.height;
    found->rgba = std::move(decoded.rgba);
    decodedBytes_ = retainedBytes + replacementBytes;
    lastError_ = TextureStagingError::None;
    return true;
}

bool TextureStagingStore::CanRefresh(const AssetRegistry& registry, std::string_view assetId) const {
    const AssetDefinition* definition = registry.Find(assetId);
    const std::vector<uint8_t>* bytes = registry.Data(assetId);
    if (definition == nullptr || bytes == nullptr || definition->kind != AssetKind::Texture || definition->state != AssetState::Ready) return false;
    const CpuTextureResource* found = Find(assetId);
    if (found == nullptr) return false;
    RgbaTexture decoded;TextureDecodeError decodeError=TextureDecodeError::None;
    const bool decodedOk=found->sourceFormat==TextureSourceFormat::PpmP6?PpmTextureDecoder::DecodeP6(*bytes,decoded,decodeError):found->sourceFormat==TextureSourceFormat::BmpBiRgb?BmpTextureDecoder::DecodeBiRgb(*bytes,decoded,decodeError):false;
    if(!decodedOk)return false;
    const size_t retainedBytes=decodedBytes_-found->rgba.size();
    return decoded.rgba.size()<=kMaxDecodedBytes-retainedBytes;
}

const CpuTextureResource* TextureStagingStore::Find(std::string_view assetId) const {
    const auto resource = std::find_if(resources_.begin(), resources_.end(), [assetId](const CpuTextureResource& value) { return value.assetId == assetId; });
    return resource == resources_.end() ? nullptr : &*resource;
}

} // namespace NeoEngine
