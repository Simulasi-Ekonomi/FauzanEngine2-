#include "Runtime/AssetResourceManager.h"
#include "Runtime/VulkanAssetUploader.h"
#include "Runtime/VulkanContext.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#define CHECK(cond, msg) do { if (!(cond)) { std::fprintf(stderr, "[FAIL] %s\n", msg); return 1; } } while (0)

static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
        if ((typeBits & (1U << i)) && (properties.memoryTypes[i].propertyFlags & required) == required) return i;
    return UINT32_MAX;
}

int main() {
    using namespace NeoEngine;
    VulkanContext context;
    if (!context.Initialize()) {
        std::printf("[INFO] Vulkan unavailable; skipping hardware uploader smoke.\n");
        return 0;
    }

    VkDevice device = context.Device();
    VkPhysicalDevice physicalDevice = context.PhysicalDevice();

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = context.GraphicsQueueFamily();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool = VK_NULL_HANDLE;
    CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &pool) == VK_SUCCESS, "command pool");

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    CHECK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) == VK_SUCCESS, "command buffer");

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent = {1U, 1U, 1U};
    imageInfo.mipLevels = 1U;
    imageInfo.arrayLayers = 1U;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage image = VK_NULL_HANDLE;
    CHECK(vkCreateImage(device, &imageInfo, nullptr, &image) == VK_SUCCESS, "image");

    VkMemoryRequirements imageRequirements{};
    vkGetImageMemoryRequirements(device, image, &imageRequirements);
    const uint32_t memoryType = FindMemoryType(physicalDevice, imageRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK(memoryType != UINT32_MAX, "device-local image memory");
    VkMemoryAllocateInfo imageAllocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    imageAllocation.allocationSize = imageRequirements.size;
    imageAllocation.memoryTypeIndex = memoryType;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    CHECK(vkAllocateMemory(device, &imageAllocation, nullptr, &imageMemory) == VK_SUCCESS, "image memory");
    CHECK(vkBindImageMemory(device, image, imageMemory, 0) == VK_SUCCESS, "bind image");

    AssetRegistry registry;
    CHECK(registry.ImportBytes("gpu.texture", AssetKind::Texture, {}, {1U, 2U, 3U, 4U}), "import texture");
    CHECK(registry.MarkReady("gpu.texture"), "ready texture");
    AssetResourceManager resources(registry);
    AssetResourceHandle handle{};
    CHECK(resources.Acquire("gpu.texture", handle), "acquire texture");

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS, "begin command buffer");

    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U};
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0U, 0U, nullptr, 0U, nullptr, 1U, &barrier);

    VulkanAssetUploader uploader(8U);
    uploader.SetPhysicalDevice(physicalDevice);
    CHECK(uploader.UploadTextureResource(resources, handle, device, commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, 1U), "record resource upload");

    AssetResourceReceipt receipt{};
    CHECK(resources.Query(handle, receipt) && !receipt.gpuResident && receipt.gpuUploadsInFlight == 1U, "upload must remain pending before fence");
    CHECK(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS, "end command buffer");

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    CHECK(vkCreateFence(device, &fenceInfo, nullptr, &fence) == VK_SUCCESS, "fence");

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1U;
    submit.pCommandBuffers = &commandBuffer;
    CHECK(vkQueueSubmit(context.GraphicsQueue(), 1U, &submit, fence) == VK_SUCCESS, "queue submit");

    uploader.AttachCompletionFence(fence, false);
    uploader.AdvanceFrame(device);
    CHECK(resources.Query(handle, receipt) && !receipt.gpuResident && receipt.gpuUploadsInFlight == 1U, "submission is not completion");

    CHECK(vkWaitForFences(device, 1U, &fence, VK_TRUE, 1000000000ULL) == VK_SUCCESS, "wait fence");
    uploader.AdvanceFrame(device);
    CHECK(resources.Query(handle, receipt) && receipt.gpuResident && receipt.gpuUploadsInFlight == 0U, "fence completion publishes residency");
    CHECK(resources.Release(handle), "release completed resource");

    vkDestroyFence(device, fence, nullptr);
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, imageMemory, nullptr);
    vkDestroyCommandPool(device, pool, nullptr);
    context.Reset();

    std::printf("VULKAN_ASSET_UPLOADER_SMOKE_OK authoritative_completion=1 residency=1 borrowed_fence=1\n");
    return 0;
}
