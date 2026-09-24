#pragma once

#include "Runtime/VulkanGPUTexture.h"
#include <vulkan/vulkan.h>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace NeoEngine
{

class TextureStreamer
{
public:
    TextureStreamer() = default;
    ~TextureStreamer();

    TextureStreamer(const TextureStreamer&) = delete;
    TextureStreamer& operator=(const TextureStreamer&) = delete;
    TextureStreamer(TextureStreamer&&) = delete;
    TextureStreamer& operator=(TextureStreamer&&) = delete;

    [[nodiscard]] bool Initialize(VkDevice device,
                                   VkPhysicalDevice physicalDevice,
                                   VkQueue graphicsQueue,
                                   uint32_t graphicsQueueFamilyIndex);

    [[nodiscard]] bool StreamIn(const std::string& key,
                                uint32_t width,
                                uint32_t height,
                                const void* pixels,
                                std::size_t byteCount);

    [[nodiscard]] bool StreamIn(const std::string& path);
    [[nodiscard]] bool StreamOut(const std::string& key);

    [[nodiscard]] bool Contains(const std::string& key) const;
    [[nodiscard]] VulkanGPUTexture* Find(const std::string& key);
    [[nodiscard]] const VulkanGPUTexture* Find(const std::string& key) const;

    void Destroy();
    [[nodiscard]] bool IsInitialized() const { return initialized_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex_ = 0;
    bool initialized_ = false;
    std::unordered_map<std::string, VulkanGPUTexture> textures_;
};

}
