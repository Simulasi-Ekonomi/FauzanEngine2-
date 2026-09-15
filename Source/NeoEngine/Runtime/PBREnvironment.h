#pragma once

#include "Runtime/PBRIBL.h"

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace NeoEngine {

struct PBREnvironmentConfig {
    uint32_t environmentFaceSize = 64;
    uint32_t irradianceFaceSize = 16;
    uint32_t prefilterFaceSize = 64;
    uint32_t prefilterSamples = 32;
    float environmentIntensity = 1.0f;
    float irradianceStrength = 1.0f;
};

class PBREnvironment {
public:
    PBREnvironment() = default;
    ~PBREnvironment();

    PBREnvironment(const PBREnvironment&) = delete;
    PBREnvironment& operator=(const PBREnvironment&) = delete;

    PBREnvironment(PBREnvironment&& other) noexcept;
    PBREnvironment& operator=(PBREnvironment&& other) noexcept;

    bool LoadHDR(const std::string& path, const PBREnvironmentConfig& config = {});
    bool UploadToVulkan();
    void Destroy();

    [[nodiscard]] bool IsCpuReady() const noexcept { return !source_.empty() && !environment_.empty(); }
    [[nodiscard]] bool IsGpuReady() const noexcept { return device_ != VK_NULL_HANDLE && environmentImage_ != VK_NULL_HANDLE && irradianceImage_ != VK_NULL_HANDLE && prefilteredImage_ != VK_NULL_HANDLE; }
    [[nodiscard]] uint32_t EnvironmentFaceSize() const noexcept { return config_.environmentFaceSize; }
    [[nodiscard]] uint32_t IrradianceFaceSize() const noexcept { return config_.irradianceFaceSize; }
    [[nodiscard]] uint32_t PrefilterFaceSize() const noexcept { return config_.prefilterFaceSize; }
    [[nodiscard]] uint32_t PrefilterMipLevels() const noexcept { return prefilterMipLevels_; }
    [[nodiscard]] const PBRIBLSettings& Settings() const noexcept { return settings_; }
    [[nodiscard]] VkImageView EnvironmentView() const noexcept { return environmentView_; }
    [[nodiscard]] VkSampler EnvironmentSampler() const noexcept { return environmentSampler_; }
    [[nodiscard]] VkImageView IrradianceView() const noexcept { return irradianceView_; }
    [[nodiscard]] VkSampler IrradianceSampler() const noexcept { return irradianceSampler_; }
    [[nodiscard]] VkImageView PrefilteredView() const noexcept { return prefilteredView_; }
    [[nodiscard]] VkSampler PrefilteredSampler() const noexcept { return prefilteredSampler_; }
    [[nodiscard]] VkFormat Format() const noexcept { return VK_FORMAT_R16G16B16A16_SFLOAT; }

private:
    struct ImageResource {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
    };

    PBREnvironmentConfig config_{};
    PBRIBLSettings settings_{};
    uint32_t sourceWidth_ = 0;
    uint32_t sourceHeight_ = 0;
    uint32_t prefilterMipLevels_ = 0;
    std::vector<glm::vec3> source_;
    std::vector<glm::vec4> environment_;
    std::vector<glm::vec4> irradiance_;
    std::vector<std::vector<glm::vec4>> prefilteredMips_;

    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = UINT32_MAX;
    ImageResource environmentResource_{};
    ImageResource irradianceResource_{};
    ImageResource prefilteredResource_{};

    VkImage environmentImage_ = VK_NULL_HANDLE;
    VkImageView environmentView_ = VK_NULL_HANDLE;
    VkSampler environmentSampler_ = VK_NULL_HANDLE;
    VkImage irradianceImage_ = VK_NULL_HANDLE;
    VkImageView irradianceView_ = VK_NULL_HANDLE;
    VkSampler irradianceSampler_ = VK_NULL_HANDLE;
    VkImage prefilteredImage_ = VK_NULL_HANDLE;
    VkImageView prefilteredView_ = VK_NULL_HANDLE;
    VkSampler prefilteredSampler_ = VK_NULL_HANDLE;
};

} // namespace NeoEngine
