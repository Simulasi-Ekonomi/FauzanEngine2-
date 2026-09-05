#include "Runtime/VulkanGPUTexture.h"
#include <cstring>
#include <utility>

namespace NeoEngine {

VulkanGPUTexture::~VulkanGPUTexture() {
    Destroy();
}

VulkanGPUTexture::VulkanGPUTexture(VulkanGPUTexture&& other) noexcept {
    device_ = other.device_;
    image_ = other.image_;
    memory_ = other.memory_;
    imageView_ = other.imageView_;
    sampler_ = other.sampler_;
    width_ = other.width_;
    height_ = other.height_;
    format_ = other.format_;

    other.device_ = VK_NULL_HANDLE;
    other.image_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.imageView_ = VK_NULL_HANDLE;
    other.sampler_ = VK_NULL_HANDLE;
    other.width_ = 0;
    other.height_ = 0;
}

VulkanGPUTexture& VulkanGPUTexture::operator=(VulkanGPUTexture&& other) noexcept {
    if (this != &other) {
        Destroy();

        device_ = other.device_;
        image_ = other.image_;
        memory_ = other.memory_;
        imageView_ = other.imageView_;
        sampler_ = other.sampler_;
        width_ = other.width_;
        height_ = other.height_;
        format_ = other.format_;

        other.device_ = VK_NULL_HANDLE;
        other.image_ = VK_NULL_HANDLE;
        other.memory_ = VK_NULL_HANDLE;
        other.imageView_ = VK_NULL_HANDLE;
        other.sampler_ = VK_NULL_HANDLE;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

uint32_t VulkanGPUTexture::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

bool VulkanGPUTexture::Initialize(VkDevice device,
                                 VkPhysicalDevice physicalDevice,
                                 uint32_t width,
                                 uint32_t height,
                                 VkFormat format) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || width == 0 || height == 0) {
        return false;
    }

    Destroy();

    device_ = device;
    width_ = width;
    height_ = height;
    format_ = format;

    // 1. Create Image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width_;
    imageInfo.extent.height = height_;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format_;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device_, &imageInfo, nullptr, &image_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    // 2. Allocate Device Memory
    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device_, image_, &memRequirements);

    uint32_t memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryTypeIndex == UINT32_MAX) {
        Destroy();
        return false;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    if (vkBindImageMemory(device_, image_, memory_, 0) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    // 3. Create Image View
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &viewInfo, nullptr, &imageView_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    return true;
}

bool VulkanGPUTexture::TransitionImageLayout(VkQueue graphicsQueue, VkCommandPool commandPool, VkImageLayout oldLayout, VkImageLayout newLayout) {
    if (device_ == VK_NULL_HANDLE || commandPool == VK_NULL_HANDLE || graphicsQueue == VK_NULL_HANDLE) {
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
    return true;
}

bool VulkanGPUTexture::UploadPixels(VkQueue graphicsQueue,
                                   uint32_t graphicsQueueFamilyIndex,
                                   VkPhysicalDevice physicalDevice,
                                   const void* pixels,
                                   VkDeviceSize dataSize) {
    if (!IsValid() || pixels == nullptr || dataSize == 0 || graphicsQueue == VK_NULL_HANDLE) {
        return false;
    }

    // Create transient CommandPool
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        return false;
    }

    // 1. Create Staging Buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        vkDestroyCommandPool(device_, commandPool, nullptr);
        return false;
    }

    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(device_, stagingBuffer, &memReqs);

    uint32_t stagingMemType = FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (stagingMemType == UINT32_MAX) {
        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkDestroyCommandPool(device_, commandPool, nullptr);
        return false;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = stagingMemType;

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkDestroyCommandPool(device_, commandPool, nullptr);
        return false;
    }

    vkBindBufferMemory(device_, stagingBuffer, stagingBufferMemory, 0);

    // 2. Map & Copy
    void* data = nullptr;
    vkMapMemory(device_, stagingBufferMemory, 0, dataSize, 0, &data);
    std::memcpy(data, pixels, static_cast<size_t>(dataSize));
    vkUnmapMemory(device_, stagingBufferMemory);

    // 3. Layout UNDEFINED -> DST_OPTIMAL
    TransitionImageLayout(graphicsQueue, commandPool, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // 4. Copy Buffer to Image
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device_, &cmdAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width_, height_, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device_, commandPool, 1, &cmd);

    // 5. Layout DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    TransitionImageLayout(graphicsQueue, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Cleanup Staging Buffer and Pool
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
    vkDestroyCommandPool(device_, commandPool, nullptr);

    return true;
}

bool VulkanGPUTexture::CreateSampler(VkFilter filter, VkSamplerAddressMode addressMode) {
    if (device_ == VK_NULL_HANDLE) {
        return false;
    }

    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void VulkanGPUTexture::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        if (sampler_ != VK_NULL_HANDLE) {
            vkDestroySampler(device_, sampler_, nullptr);
            sampler_ = VK_NULL_HANDLE;
        }
        if (imageView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, imageView_, nullptr);
            imageView_ = VK_NULL_HANDLE;
        }
        if (image_ != VK_NULL_HANDLE) {
            vkDestroyImage(device_, image_, nullptr);
            image_ = VK_NULL_HANDLE;
        }
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_, memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
        }
        device_ = VK_NULL_HANDLE;
    }
    width_ = 0;
    height_ = 0;
}

} // namespace NeoEngine
