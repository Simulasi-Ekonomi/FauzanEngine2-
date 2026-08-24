#include "VulkanTextureUpload.h"

#include "VulkanContext.h"

#include <cstring>
#include <limits>
#include <vulkan/vulkan.h>

namespace NeoEngine {
namespace {

constexpr VkFormat kTextureFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;
constexpr VkDeviceSize kMaxTextureBytes = 64ULL * 1024ULL * 1024ULL;

uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeBits, VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(device, &properties);
    for (uint32_t index = 0; index < properties.memoryTypeCount; ++index) {
        if ((typeBits & (1U << index)) != 0 && (properties.memoryTypes[index].propertyFlags & required) == required) return index;
    }
    return UINT32_MAX;
}

uint64_t HashBytes(const uint8_t* bytes, size_t count) {
    uint64_t hash = kHashOffset;
    for (size_t index = 0; index < count; ++index) {
        hash ^= bytes[index];
        hash *= kHashPrime;
    }
    return hash;
}

struct TextureUploadResources {
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkBuffer uploadBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uploadMemory = VK_NULL_HANDLE;
    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureMemory = VK_NULL_HANDLE;
    VkBuffer readbackBuffer = VK_NULL_HANDLE;
    VkDeviceMemory readbackMemory = VK_NULL_HANDLE;

    ~TextureUploadResources() {
        if (device == VK_NULL_HANDLE) return;
        vkDeviceWaitIdle(device);
        if (fence != VK_NULL_HANDLE) vkDestroyFence(device, fence, nullptr);
        if (readbackBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, readbackBuffer, nullptr);
        if (readbackMemory != VK_NULL_HANDLE) vkFreeMemory(device, readbackMemory, nullptr);
        if (textureImage != VK_NULL_HANDLE) vkDestroyImage(device, textureImage, nullptr);
        if (textureMemory != VK_NULL_HANDLE) vkFreeMemory(device, textureMemory, nullptr);
        if (uploadBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, uploadBuffer, nullptr);
        if (uploadMemory != VK_NULL_HANDLE) vkFreeMemory(device, uploadMemory, nullptr);
        if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, commandPool, nullptr);
    }
};

bool CreateHostBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) return false;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    const uint32_t memoryType = FindMemoryType(physicalDevice, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == UINT32_MAX) return false;
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;
    return vkAllocateMemory(device, &allocation, nullptr, &memory) == VK_SUCCESS && vkBindBufferMemory(device, buffer, memory, 0) == VK_SUCCESS;
}

} // namespace

VulkanTextureUploadResult VulkanTextureUploader::UploadAndReadback(const RgbaTexture& texture) {
    VulkanTextureUploadResult result{};
    if (texture.width == 0 || texture.height == 0) return result;
    const uint64_t byteCount64 = static_cast<uint64_t>(texture.width) * texture.height * 4U;
    if (byteCount64 == 0 || byteCount64 > kMaxTextureBytes || byteCount64 > std::numeric_limits<VkDeviceSize>::max() || texture.rgba.size() != byteCount64) return result;
    const VkDeviceSize byteCount = static_cast<VkDeviceSize>(byteCount64);

    VulkanContext context;
    if (!context.Initialize()) return result;
    result.deviceCreated = true;
    result.width = texture.width;
    result.height = texture.height;

    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(context.PhysicalDevice(), kTextureFormat, &formatProperties);
    if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0) return result;

    TextureUploadResources resources{};
    resources.device = context.Device();
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = context.GraphicsQueueFamily();
    if (vkCreateCommandPool(resources.device, &poolInfo, nullptr, &resources.commandPool) != VK_SUCCESS) return result;
    VkCommandBufferAllocateInfo commandBufferInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferInfo.commandPool = resources.commandPool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(resources.device, &commandBufferInfo, &resources.commandBuffer) != VK_SUCCESS) return result;
    if (!CreateHostBuffer(context.PhysicalDevice(), resources.device, byteCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, resources.uploadBuffer, resources.uploadMemory) ||
        !CreateHostBuffer(context.PhysicalDevice(), resources.device, byteCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT, resources.readbackBuffer, resources.readbackMemory)) return result;

    void* uploadMapped = nullptr;
    if (vkMapMemory(resources.device, resources.uploadMemory, 0, byteCount, 0, &uploadMapped) != VK_SUCCESS || uploadMapped == nullptr) return result;
    std::memcpy(uploadMapped, texture.rgba.data(), static_cast<size_t>(byteCount));
    vkUnmapMemory(resources.device, resources.uploadMemory);
    result.uploadHash = HashBytes(texture.rgba.data(), texture.rgba.size());

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = kTextureFormat;
    imageInfo.extent = {texture.width, texture.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(resources.device, &imageInfo, nullptr, &resources.textureImage) != VK_SUCCESS) return result;
    VkMemoryRequirements imageRequirements{};
    vkGetImageMemoryRequirements(resources.device, resources.textureImage, &imageRequirements);
    const uint32_t imageMemoryType = FindMemoryType(context.PhysicalDevice(), imageRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (imageMemoryType == UINT32_MAX) return result;
    VkMemoryAllocateInfo imageAllocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    imageAllocation.allocationSize = imageRequirements.size;
    imageAllocation.memoryTypeIndex = imageMemoryType;
    if (vkAllocateMemory(resources.device, &imageAllocation, nullptr, &resources.textureMemory) != VK_SUCCESS ||
        vkBindImageMemory(resources.device, resources.textureImage, resources.textureMemory, 0) != VK_SUCCESS) return result;
    result.textureAllocated = true;

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(resources.commandBuffer, &beginInfo) != VK_SUCCESS) return result;
    VkImageMemoryBarrier toTransferDst{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toTransferDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferDst.image = resources.textureImage;
    toTransferDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransferDst.subresourceRange.levelCount = 1;
    toTransferDst.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(resources.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransferDst);

    VkBufferImageCopy uploadRegion{};
    uploadRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    uploadRegion.imageSubresource.layerCount = 1;
    uploadRegion.imageExtent = {texture.width, texture.height, 1};
    vkCmdCopyBufferToImage(resources.commandBuffer, resources.uploadBuffer, resources.textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &uploadRegion);

    VkImageMemoryBarrier toTransferSrc{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    toTransferSrc.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toTransferSrc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toTransferSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferSrc.image = resources.textureImage;
    toTransferSrc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransferSrc.subresourceRange.levelCount = 1;
    toTransferSrc.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(resources.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransferSrc);

    VkBufferImageCopy readbackRegion{};
    readbackRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    readbackRegion.imageSubresource.layerCount = 1;
    readbackRegion.imageExtent = {texture.width, texture.height, 1};
    vkCmdCopyImageToBuffer(resources.commandBuffer, resources.textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, resources.readbackBuffer, 1, &readbackRegion);
    VkBufferMemoryBarrier readbackBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    readbackBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    readbackBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    readbackBarrier.buffer = resources.readbackBuffer;
    readbackBarrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(resources.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &readbackBarrier, 0, nullptr);
    if (vkEndCommandBuffer(resources.commandBuffer) != VK_SUCCESS) return result;

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vkCreateFence(resources.device, &fenceInfo, nullptr, &resources.fence) != VK_SUCCESS) return result;
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &resources.commandBuffer;
    if (vkQueueSubmit(context.GraphicsQueue(), 1, &submitInfo, resources.fence) != VK_SUCCESS || vkWaitForFences(resources.device, 1, &resources.fence, VK_TRUE, 5'000'000'000ULL) != VK_SUCCESS) return result;
    result.commandSubmitted = true;

    void* readbackMapped = nullptr;
    if (vkMapMemory(resources.device, resources.readbackMemory, 0, byteCount, 0, &readbackMapped) != VK_SUCCESS || readbackMapped == nullptr) return result;
    result.readbackHash = HashBytes(static_cast<const uint8_t*>(readbackMapped), static_cast<size_t>(byteCount));
    vkUnmapMemory(resources.device, resources.readbackMemory);
    result.pixelsReadback = result.readbackHash == result.uploadHash && result.readbackHash != 0;
    return result;
}

} // namespace NeoEngine
