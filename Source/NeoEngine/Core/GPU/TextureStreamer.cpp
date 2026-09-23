#include "TextureStreamer.h"

#include "Runtime/PpmTexture.h"

#include <fstream>
#include <iterator>
#include <vector>

namespace NeoEngine {

TextureStreamer::~TextureStreamer() {
    Destroy();
}

bool TextureStreamer::Initialize(VkDevice device, VkPhysicalDevice physicalDevice,
                                 VkQueue graphicsQueue, uint32_t graphicsQueueFamily) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE ||
        graphicsQueue == VK_NULL_HANDLE || graphicsQueueFamily == UINT32_MAX) {
        return false;
    }
    Destroy();
    device_ = device;
    physicalDevice_ = physicalDevice;
    graphicsQueue_ = graphicsQueue;
    graphicsQueueFamily_ = graphicsQueueFamily;
    return true;
}

bool TextureStreamer::StreamIn(const std::string& key, uint32_t width, uint32_t height,
                               const void* rgbaPixels, std::size_t byteCount) {
    if (!IsInitialized() || key.empty() || width == 0 || height == 0 ||
        rgbaPixels == nullptr || byteCount != static_cast<std::size_t>(width) * height * 4U) {
        return false;
    }

    VulkanGPUTexture texture;
    if (!texture.Initialize(device_, physicalDevice_, width, height, VK_FORMAT_R8G8B8A8_UNORM)) {
        return false;
    }
    if (!texture.UploadPixels(graphicsQueue_, graphicsQueueFamily_, physicalDevice_,
                              rgbaPixels, static_cast<VkDeviceSize>(byteCount))) {
        return false;
    }
    if (!texture.CreateSampler()) {
        return false;
    }

    auto [it, inserted] = textures_.try_emplace(key, std::move(texture));
    if (!inserted) it->second = std::move(texture);
    return true;
}

bool TextureStreamer::StreamIn(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
    RgbaTexture texture;
    TextureDecodeError error = TextureDecodeError::None;
    if (!PpmTextureDecoder::DecodeP6(bytes, texture, error)) return false;

    return StreamIn(path, texture.width, texture.height, texture.rgba.data(), texture.rgba.size());
}

bool TextureStreamer::StreamOut(const std::string& path) {
    if (!IsInitialized()) return false;
    return textures_.erase(path) != 0U;
}

const VulkanGPUTexture* TextureStreamer::Find(const std::string& key) const noexcept {
    const auto it = textures_.find(key);
    return it == textures_.end() ? nullptr : &it->second;
}

void TextureStreamer::Destroy() {
    textures_.clear();
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    graphicsQueue_ = VK_NULL_HANDLE;
    graphicsQueueFamily_ = UINT32_MAX;
}

} // namespace NeoEngine
