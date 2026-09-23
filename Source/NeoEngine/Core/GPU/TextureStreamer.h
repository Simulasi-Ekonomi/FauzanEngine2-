#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "Runtime/VulkanGPUTexture.h"

namespace NeoEngine {

class TextureStreamer {
public:
    TextureStreamer() = default;
    ~TextureStreamer();

    TextureStreamer(const TextureStreamer&) = delete;
    TextureStreamer& operator=(const TextureStreamer&) = delete;
    TextureStreamer(TextureStreamer&&) noexcept = default;
    TextureStreamer& operator=(TextureStreamer&&) noexcept = default;

    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice,
                    VkQueue graphicsQueue, uint32_t graphicsQueueFamily);
    bool StreamIn(const std::string& path);
    bool StreamIn(const std::string& key, uint32_t width, uint32_t height,
                  const void* rgbaPixels, std::size_t byteCount);
    bool StreamOut(const std::string& path);

    [[nodiscard]] bool IsInitialized() const noexcept { return device_ != VK_NULL_HANDLE; }
    [[nodiscard]] bool Contains(const std::string& key) const noexcept { return textures_.find(key) != textures_.end(); }
    [[nodiscard]] const VulkanGPUTexture* Find(const std::string& key) const noexcept;

    void Destroy();

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = UINT32_MAX;
    std::unordered_map<std::string, VulkanGPUTexture> textures_;
};

} // namespace NeoEngine
