#include "Runtime/VulkanAssetUploader.h"
#include "Runtime/VulkanContext.h"

#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\\n"; \
            return 1; \
        } \
    } while (0)

namespace {

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice,
                        uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) != 0u &&
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return std::numeric_limits<uint32_t>::max();
}

bool CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) return false;

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    const uint32_t memoryType = FindMemoryType(
        physicalDevice, requirements.memoryTypeBits, properties);
    if (memoryType == std::numeric_limits<uint32_t>::max()) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return false;
    }

    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;

    if (vkAllocateMemory(device, &allocation, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return false;
    }
    if (vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

bool ReadBuffer(VkDevice device, VkDeviceMemory memory, VkDeviceSize size,
                std::vector<uint8_t>& output) {
    output.resize(static_cast<size_t>(size));
    void* mapped = nullptr;
    if (vkMapMemory(device, memory, 0, size, 0, &mapped) != VK_SUCCESS) return false;
    std::memcpy(output.data(), mapped, static_cast<size_t>(size));
    vkUnmapMemory(device, memory);
    return true;
}

} // namespace

int main() {
    std::cout << "[Smoke Test] Starting vulkan_asset_uploader_smoke...\\n";

    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cout << "[INFO] VulkanContext unavailable; skipping hardware uploader smoke.\\n";
        return 0;
    }

    const VkDevice device = context.Device();
    const VkPhysicalDevice physicalDevice = context.PhysicalDevice();
    const VkQueue queue = context.GraphicsQueue();
    const uint32_t queueFamily = context.GraphicsQueueFamily();

    TEST_CHECK(device != VK_NULL_HANDLE, "Vulkan device unavailable");
    TEST_CHECK(physicalDevice != VK_NULL_HANDLE, "Vulkan physical device unavailable");
    TEST_CHECK(queue != VK_NULL_HANDLE, "Vulkan graphics queue unavailable");

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamily;
    TEST_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS,
               "Command pool creation failed");

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = commandPool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = 1;
    TEST_CHECK(vkAllocateCommandBuffers(device, &commandInfo, &commandBuffer) == VK_SUCCESS,
               "Command buffer allocation failed");

    const std::vector<uint8_t> vertexData{
        0x10, 0x20, 0x30, 0x40,
        0x50, 0x60, 0x70, 0x80,
        0x90, 0xA0, 0xB0, 0xC0
    };
    const std::vector<uint8_t> indexData{
        0x01, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00
    };

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;

    TEST_CHECK(CreateBuffer(
        device, physicalDevice, vertexData.size(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBuffer, vertexMemory), "Vertex target buffer creation failed");

    TEST_CHECK(CreateBuffer(
        device, physicalDevice, indexData.size(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexBuffer, indexMemory), "Index target buffer creation failed");

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    TEST_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &fence) == VK_SUCCESS,
               "Completion fence creation failed");

    NeoEngine::VulkanAssetUploader uploader(8);
    TEST_CHECK(uploader.Initialize(device, physicalDevice), "Uploader initialization failed");
    TEST_CHECK(uploader.GetCurrentStagingUsedMB() == 0, "Uploader initial budget is not empty");

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    TEST_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS,
               "Command buffer begin failed");

    TEST_CHECK(uploader.UploadMesh(
        device, commandBuffer, vertexData, indexData,
        vertexBuffer, indexBuffer, fence), "UploadMesh failed");

    TEST_CHECK(uploader.GetPendingUploadCount() == 1,
               "Upload task was not retained until fence completion");
    TEST_CHECK(uploader.GetCurrentStagingUsedMB() >= 1,
               "Staging budget was not reserved");

    TEST_CHECK(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS,
               "Command buffer end failed");

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;
    TEST_CHECK(vkQueueSubmit(queue, 1, &submit, fence) == VK_SUCCESS,
               "Upload submission failed");

    TEST_CHECK(vkWaitForFences(
        device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max()) == VK_SUCCESS,
        "Upload fence wait failed");

    std::vector<uint8_t> readVertexData;
    std::vector<uint8_t> readIndexData;
    TEST_CHECK(ReadBuffer(device, vertexMemory, vertexData.size(), readVertexData),
               "Vertex target readback failed");
    TEST_CHECK(ReadBuffer(device, indexMemory, indexData.size(), readIndexData),
               "Index target readback failed");
    TEST_CHECK(readVertexData == vertexData, "Vertex upload data mismatch");
    TEST_CHECK(readIndexData == indexData, "Index upload data mismatch");

    uploader.AdvanceFrame(device);
    TEST_CHECK(uploader.GetPendingUploadCount() == 0,
               "Completed staging task was not released");
    TEST_CHECK(uploader.GetCurrentStagingUsedMB() == 0,
               "Completed staging budget was not released");

    uploader.Shutdown(device);
    vkDestroyFence(device, fence, nullptr);
    vkFreeMemory(device, indexMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, vertexMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    context.Reset();

    std::cout << "VULKAN_ASSET_UPLOADER_OK mesh_copy=fence_backed\\n";
    return 0;
}
