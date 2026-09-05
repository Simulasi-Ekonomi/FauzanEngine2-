#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NeoEngine {

struct DescriptorLayoutBindingInfo {
    uint32_t binding = 0;
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uint32_t descriptorCount = 1;
};

class VulkanDescriptorManager {
public:
    VulkanDescriptorManager() = default;
    ~VulkanDescriptorManager();

    VulkanDescriptorManager(const VulkanDescriptorManager&) = delete;
    VulkanDescriptorManager& operator=(const VulkanDescriptorManager&) = delete;

    VulkanDescriptorManager(VulkanDescriptorManager&& other) noexcept;
    VulkanDescriptorManager& operator=(VulkanDescriptorManager&& other) noexcept;

    bool Initialize(VkDevice device,
                    const std::vector<DescriptorLayoutBindingInfo>& bindings,
                    uint32_t maxSets = 16);

    VkDescriptorSet AllocateSet();

    void UpdateBufferBinding(VkDescriptorSet descriptorSet,
                             uint32_t binding,
                             VkDescriptorType type,
                             VkBuffer buffer,
                             VkDeviceSize offset,
                             VkDeviceSize range);

    void UpdateImageBinding(VkDescriptorSet descriptorSet,
                            uint32_t binding,
                            VkImageView imageView,
                            VkSampler sampler,
                            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    void Destroy();

    [[nodiscard]] VkDescriptorSetLayout GetLayout() const { return layout_; }
    [[nodiscard]] VkDescriptorPool GetPool() const { return pool_; }
    [[nodiscard]] bool IsValid() const { return layout_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
};

} // namespace NeoEngine
