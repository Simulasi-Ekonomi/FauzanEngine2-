#include "Runtime/AssetRegistry.h"
#include "Runtime/TextureStaging.h"

#include <cstdio>

namespace {
void U16(std::vector<uint8_t>& bytes,uint16_t value){bytes.push_back(static_cast<uint8_t>(value));bytes.push_back(static_cast<uint8_t>(value>>8U));}
void U32(std::vector<uint8_t>& bytes,uint32_t value){for(uint8_t shift=0;shift<32U;shift+=8U)bytes.push_back(static_cast<uint8_t>(value>>shift));}
std::vector<uint8_t> Bmp24(){std::vector<uint8_t> bytes{'B','M'};U32(bytes,62U);U16(bytes,0U);U16(bytes,0U);U32(bytes,54U);U32(bytes,40U);U32(bytes,2U);U32(bytes,1U);U16(bytes,1U);U16(bytes,24U);U32(bytes,0U);U32(bytes,8U);U32(bytes,0U);U32(bytes,0U);U32(bytes,0U);U32(bytes,0U);bytes.insert(bytes.end(),{0U,0U,255U,0U,255U,0U,0U,0U});return bytes;}
}
int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', 10, 20, 30};
    AssetRegistry registry;
    if (!registry.ImportBytes("tile.ppm", AssetKind::Texture, {}, ppm) || !registry.MarkReady("tile.ppm")) return 1;
    TextureStagingStore staging;
    if (!staging.StagePpm(registry, "tile.ppm")) return 1;
    const CpuTextureResource* resource = staging.Find("tile.ppm");
    if (resource == nullptr || !staging.IsCurrent(registry,"tile.ppm") || resource->width != 1 || resource->height != 1 || resource->rgba.size() != 4 || resource->rgba[0] != 10 || resource->rgba[3] != 255 ||
        staging.StagePpm(registry, "tile.ppm") || staging.LastError() != TextureStagingError::DuplicateResource) return 1;
    const uint64_t ppmHash=resource->sourceHash;const std::vector<uint8_t> bmp=Bmp24();
    if(!registry.ImportBytes("tile.bmp",AssetKind::Texture,{},bmp)||!registry.MarkReady("tile.bmp")||!staging.StageBmp(registry,"tile.bmp"))return 1;
    const CpuTextureResource* bmpResource=staging.Find("tile.bmp");
    if(!bmpResource||!staging.IsCurrent(registry,"tile.bmp")||bmpResource->width!=2U||bmpResource->height!=1U||bmpResource->rgba.size()!=8U||bmpResource->rgba[0U]!=255U||bmpResource->rgba[5U]!=255U||resource->assetId!="tile.ppm"||resource->sourceHash!=ppmHash||resource->rgba[0U]!=10U||staging.StageBmp(registry,"tile.bmp")||staging.LastError()!=TextureStagingError::DuplicateResource)return 1;
    const std::vector<uint8_t> replacementPpm{'P','6','\n','1',' ','1','\n','2','5','5','\n',40,50,60};if(!registry.ReplaceBytes("tile.ppm",replacementPpm)||staging.IsCurrent(registry,"tile.ppm")||!staging.IsCurrent(registry,"tile.bmp")||!staging.Refresh(registry,"tile.ppm")||staging.Find("tile.ppm")!=resource||!staging.IsCurrent(registry,"tile.ppm")||resource->sourceHash==ppmHash||resource->sourceFormat!=TextureSourceFormat::PpmP6||resource->rgba[0U]!=40U||resource->rgba[3U]!=255U)return 1;
    std::vector<uint8_t> replacementBmp=bmp;replacementBmp[54U]=255U;replacementBmp[56U]=0U;const uint64_t bmpHash=bmpResource->sourceHash;if(!registry.ReplaceBytes("tile.bmp",replacementBmp)||staging.IsCurrent(registry,"tile.bmp")||!staging.Refresh(registry,"tile.bmp")||staging.Find("tile.bmp")!=bmpResource||!staging.IsCurrent(registry,"tile.bmp")||bmpResource->sourceHash==bmpHash||bmpResource->sourceFormat!=TextureSourceFormat::BmpBiRgb||bmpResource->rgba[0U]!=0U||bmpResource->rgba[2U]!=255U)return 1;
    const uint64_t refreshedPpmHash=resource->sourceHash;const std::vector<uint8_t> refreshFailurePpm{'P','6','\n','1',' ','1','\n','2','5','5','\n',10};if(!registry.ReplaceBytes("tile.ppm",refreshFailurePpm)||staging.IsCurrent(registry,"tile.ppm")||staging.Refresh(registry,"tile.ppm")||staging.LastError()!=TextureStagingError::DecodeFailed||staging.Find("tile.ppm")!=resource||resource->sourceHash!=refreshedPpmHash||resource->rgba[0U]!=40U||staging.IsCurrent(registry,"tile.ppm"))return 1;
    if (!registry.ImportBytes("mesh.ppm", AssetKind::Mesh, {}, ppm) || !registry.MarkReady("mesh.ppm") || staging.StagePpm(registry, "mesh.ppm") || staging.LastError() != TextureStagingError::WrongKind) return 1;
    const std::vector<uint8_t> badPpm{'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', 10};
    if (!registry.ImportBytes("bad.ppm", AssetKind::Texture, {}, badPpm) || !registry.MarkReady("bad.ppm") || staging.StagePpm(registry, "bad.ppm") || staging.LastError() != TextureStagingError::DecodeFailed) return 1;
    const std::vector<uint8_t> badBmp{'B','M',0};
    if(!registry.ImportBytes("bad.bmp",AssetKind::Texture,{},badBmp)||!registry.MarkReady("bad.bmp")||staging.StageBmp(registry,"bad.bmp")||staging.LastError()!=TextureStagingError::DecodeFailed)return 1;
    std::printf("TEXTURE_STAGING_SMOKE_OK textures=%zu decodedBytes=%zu bmp=1 staleDetection=1 refresh=1 stablePointer=1 hash=%llu\n", staging.ResourceCount(), staging.DecodedBytes(), static_cast<unsigned long long>(ppmHash));
    return 0;
}
