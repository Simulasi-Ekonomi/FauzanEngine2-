#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>

namespace NeoEngine {

class VulkanGPUTexture {
public:
    VulkanGPUTexture() = default;
    ~VulkanGPUTexture();

    VulkanGPUTexture(const VulkanGPUTexture&) = delete;
    VulkanGPUTexture& operator=(const VulkanGPUTexture&) = delete;

    VulkanGPUTexture(VulkanGPUTexture&& other) noexcept;
    VulkanGPUTexture& operator=(VulkanGPUTexture&& other) noexcept;

    bool Initialize(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    uint32_t width,
                    uint32_t height,
                    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);

    bool UploadPixels(VkQueue graphicsQueue,
                      uint32_t graphicsQueueFamilyIndex,
                      VkPhysicalDevice physicalDevice,
                      const void* pixels,
                      VkDeviceSize dataSize);

    bool CreateSampler(VkFilter filter = VK_FILTER_LINEAR,
                       VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    void Destroy();

    [[nodiscard]] VkImage GetImage() const { return image_; }
    [[nodiscard]] VkDeviceMemory GetMemory() const { return memory_; }
    [[nodiscard]] VkImageView GetImageView() const { return imageView_; }
    [[nodiscard]] VkSampler GetSampler() const { return sampler_; }
    [[nodiscard]] uint32_t GetWidth() const { return width_; }
    [[nodiscard]] uint32_t GetHeight() const { return height_; }
    [[nodiscard]] VkFormat GetFormat() const { return format_; }
    [[nodiscard]] bool IsValid() const { return image_ != VK_NULL_HANDLE && memory_ != VK_NULL_HANDLE && imageView_ != VK_NULL_HANDLE; }

private:
    uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool TransitionImageLayout(VkQueue graphicsQueue, VkCommandPool commandPool, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkDevice device_ = VK_NULL_HANDLE;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView imageView_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    VkFormat format_ = VK_FORMAT_R8G8B8A8_UNORM;
};

} // namespace NeoEngine
