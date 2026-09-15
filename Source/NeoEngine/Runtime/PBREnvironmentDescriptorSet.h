#pragma once

#include "Runtime/PBREnvironment.h"
#include "Runtime/VulkanDescriptorManager.h"

#include <cstdint>
#include <vulkan/vulkan.h>

namespace NeoEngine {

class PBREnvironmentDescriptorSet {
public:
    PBREnvironmentDescriptorSet() = default;
    ~PBREnvironmentDescriptorSet() = default;

    PBREnvironmentDescriptorSet(const PBREnvironmentDescriptorSet&) = delete;
    PBREnvironmentDescriptorSet& operator=(const PBREnvironmentDescriptorSet&) = delete;
    PBREnvironmentDescriptorSet(PBREnvironmentDescriptorSet&&) noexcept = default;
    PBREnvironmentDescriptorSet& operator=(PBREnvironmentDescriptorSet&&) noexcept = default;

    bool Initialize(VkDevice device, uint32_t maxSets = 16);
    VkDescriptorSet AllocateSet();

    void Update(VkDescriptorSet descriptorSet,
                const PBREnvironment& environment,
                VkImageView brdfLutView,
                VkSampler brdfLutSampler);

    void Destroy();

    [[nodiscard]] VkDescriptorSetLayout GetLayout() const { return descriptors_.GetLayout(); }
    [[nodiscard]] bool IsValid() const { return descriptors_.IsValid(); }

private:
    VulkanDescriptorManager descriptors_;
};

} // namespace NeoEngine
