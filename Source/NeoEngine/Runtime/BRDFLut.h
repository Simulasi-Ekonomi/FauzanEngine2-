#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Runtime/VulkanGPUTexture.h"

namespace NeoEngine {

class BRDFLut {
public:
    static constexpr uint32_t LUT_RESOLUTION = 512;
    static constexpr VkFormat LUT_FORMAT = VK_FORMAT_R16G16_SFLOAT;

    BRDFLut() noexcept = default;
    ~BRDFLut() noexcept = default;

    BRDFLut(const BRDFLut&) = delete;
    BRDFLut& operator=(const BRDFLut&) = delete;
    BRDFLut(BRDFLut&&) noexcept = default;
    BRDFLut& operator=(BRDFLut&&) noexcept = default;

    // Supplies the Vulkan context used by Generate(). The object does not own the device or queue.
    [[nodiscard]] bool Initialize(VkDevice device,
                                   VkPhysicalDevice physicalDevice,
                                   VkQueue graphicsQueue,
                                   uint32_t graphicsQueueFamily) noexcept;

    // Generates the GGX/Smith split-sum LUT and uploads it to a sampled Vulkan image.
    [[nodiscard]] bool Generate() noexcept;
    void Destroy() noexcept;

    [[nodiscard]] VkImageView GetImageView() const noexcept { return texture_.GetImageView(); }
    [[nodiscard]] VkImage GetImage() const noexcept { return texture_.GetImage(); }
    [[nodiscard]] VkDeviceMemory GetMemory() const noexcept { return texture_.GetMemory(); }
    [[nodiscard]] VkSampler GetSampler() const noexcept { return texture_.GetSampler(); }
    [[nodiscard]] bool IsValid() const noexcept { return isValid_ && texture_.IsValid(); }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = UINT32_MAX;
    VulkanGPUTexture texture_;
    bool isValid_ = false;

    [[nodiscard]] static glm::vec2 IntegrateBRDF(float roughness, float ndotv) noexcept;
};

} // namespace NeoEngine
