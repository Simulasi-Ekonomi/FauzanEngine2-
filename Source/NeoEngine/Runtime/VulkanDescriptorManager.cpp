#include "Runtime/VulkanDescriptorManager.h"
#include <utility>

namespace NeoEngine {

VulkanDescriptorManager::~VulkanDescriptorManager() {
    Destroy();
}

VulkanDescriptorManager::VulkanDescriptorManager(VulkanDescriptorManager&& other) noexcept {
    device_ = other.device_;
    layout_ = other.layout_;
    pool_ = other.pool_;

    other.device_ = VK_NULL_HANDLE;
    other.layout_ = VK_NULL_HANDLE;
    other.pool_ = VK_NULL_HANDLE;
}

VulkanDescriptorManager& VulkanDescriptorManager::operator=(VulkanDescriptorManager&& other) noexcept {
    if (this != &other) {
        Destroy();

        device_ = other.device_;
        layout_ = other.layout_;
        pool_ = other.pool_;

        other.device_ = VK_NULL_HANDLE;
        other.layout_ = VK_NULL_HANDLE;
        other.pool_ = VK_NULL_HANDLE;
    }
    return *this;
}

bool VulkanDescriptorManager::Initialize(VkDevice device,
                                         const std::vector<DescriptorLayoutBindingInfo>& bindings,
                                         uint32_t maxSets) {
    if (device == VK_NULL_HANDLE || bindings.empty() || maxSets == 0) {
        return false;
    }

    Destroy();
    device_ = device;

    // 1. Create Descriptor Set Layout
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    layoutBindings.reserve(bindings.size());

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(bindings.size());

    for (const auto& binding : bindings) {
        VkDescriptorSetLayoutBinding b{};
        b.binding = binding.binding;
        b.descriptorType = binding.type;
        b.descriptorCount = binding.descriptorCount;
        b.stageFlags = binding.stageFlags;
        b.pImmutableSamplers = nullptr;
        layoutBindings.push_back(b);

        VkDescriptorPoolSize pSize{};
        pSize.type = binding.type;
        pSize.descriptorCount = binding.descriptorCount * maxSets;
        poolSizes.push_back(pSize);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &layout_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    // 2. Create Descriptor Pool
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &pool_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    return true;
}

VkDescriptorSet VulkanDescriptorManager::AllocateSet() {
    if (!IsValid()) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout_;

    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(device_, &allocInfo, &set) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return set;
}

void VulkanDescriptorManager::UpdateBufferBinding(VkDescriptorSet descriptorSet,
                                                  uint32_t binding,
                                                  VkDescriptorType type,
                                                  VkBuffer buffer,
                                                  VkDeviceSize offset,
                                                  VkDeviceSize range) {
    if (device_ == VK_NULL_HANDLE || descriptorSet == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = type;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
}

void VulkanDescriptorManager::UpdateImageBinding(VkDescriptorSet descriptorSet,
                                                 uint32_t binding,
                                                 VkImageView imageView,
                                                 VkSampler sampler,
                                                 VkImageLayout imageLayout) {
    if (device_ == VK_NULL_HANDLE || descriptorSet == VK_NULL_HANDLE || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = imageLayout;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
}

void VulkanDescriptorManager::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        if (pool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device_, pool_, nullptr);
            pool_ = VK_NULL_HANDLE;
        }
        if (layout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, layout_, nullptr);
            layout_ = VK_NULL_HANDLE;
        }
        device_ = VK_NULL_HANDLE;
    }
}

} // namespace NeoEngine
