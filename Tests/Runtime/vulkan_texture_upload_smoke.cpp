#include "Runtime/AssetRegistry.h"
#include "Runtime/TextureStaging.h"
#include "Runtime/VulkanTextureUpload.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{
        'P', '6', '\n', '2', ' ', '2', '\n', '2', '5', '5', '\n',
        255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255,
    };
    AssetRegistry registry;
    TextureStagingStore staging;
    if (!registry.ImportBytes("gpu-tile.ppm", AssetKind::Texture, {}, ppm) || !registry.MarkReady("gpu-tile.ppm") || !staging.StagePpm(registry, "gpu-tile.ppm")) return 1;
    const CpuTextureResource* resource = staging.Find("gpu-tile.ppm");
    if (resource == nullptr) return 1;
    RgbaTexture texture{resource->width, resource->height, resource->rgba};
    const VulkanTextureUploadResult invalid = VulkanTextureUploader::UploadAndReadback({});
    const VulkanTextureUploadResult first = VulkanTextureUploader::UploadAndReadback(texture);
    const VulkanTextureUploadResult second = VulkanTextureUploader::UploadAndReadback(texture);
    if (invalid.deviceCreated || !first.deviceCreated || !first.textureAllocated || !first.commandSubmitted || !first.pixelsReadback || first.width != 2 || first.height != 2 ||
        first.uploadHash == 0 || first.uploadHash != first.readbackHash || !second.pixelsReadback || second.readbackHash != first.readbackHash) {
        return 1;
    }
    std::printf("VULKAN_TEXTURE_UPLOAD_SMOKE_OK size=%ux%u hash=%llu staged=%zu\n", first.width, first.height, static_cast<unsigned long long>(first.readbackHash), staging.DecodedBytes());
    return 0;
}
