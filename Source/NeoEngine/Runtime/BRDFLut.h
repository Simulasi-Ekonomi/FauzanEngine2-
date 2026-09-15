#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>

namespace NeoEngine {

class BRDFLut {
public:
    static constexpr uint32_t LUT_RESOLUTION = 512;
    static constexpr VkFormat LUT_FORMAT = VK_FORMAT_R16G16_SFLOAT;

    BRDFLut() noexcept = default;
    ~BRDFLut() noexcept;

    BRDFLut(const BRDFLut&) = delete;
    BRDFLut& operator=(const BRDFLut&) = delete;
    BRDFLut(BRDFLut&&) noexcept = default;
    BRDFLut& operator=(BRDFLut&&) noexcept = default;

    // Generate BRDF lookup table texture
    // Pre-computes BRDF for all roughness/view angle combinations
    [[nodiscard]] bool Generate() noexcept;

    // Get the LUT texture image view for GPU binding
    [[nodiscard]] VkImageView GetImageView() const noexcept { return imageView_; }

    // Get the LUT texture image for memory management
    [[nodiscard]] VkImage GetImage() const noexcept { return image_; }

    // Check if LUT is valid and ready to use
    [[nodiscard]] bool IsValid() const noexcept { return isValid_; }

private:
    VkImage image_ = VK_NULL_HANDLE;
    VkImageView imageView_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    bool isValid_ = false;

    // Cook-Torrance BRDF calculation (CPU-side for LUT generation)
    [[nodiscard]] static glm::vec2 IntegrateBRDF(float roughness, float ndotv) noexcept;
};

} // namespace NeoEngine
