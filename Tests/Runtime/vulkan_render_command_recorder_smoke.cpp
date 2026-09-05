#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanGPUBuffer.h"
#include "Runtime/VulkanRenderCommandRecorder.h"
#include <iostream>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting vulkan_render_command_recorder_smoke...\n";

    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cout << "[INFO] VulkanContext failed to initialize (likely headless environment). Skipping hardware test gracefully.\n";
        return 0;
    }

    VkDevice device = context.Device();
    VkPhysicalDevice physicalDevice = context.PhysicalDevice();
    uint32_t queueFamily = context.GraphicsQueueFamily();

    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        std::cout << "[INFO] No valid Vulkan physical or logical device available. Skipping test gracefully.\n";
        return 0;
    }

    // 1. Create Command Pool
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult poolRes = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    TEST_CHECK(poolRes == VK_SUCCESS, "CommandPool creation failed");

    // 2. Create Dummy Vertex & Index Buffers
    NeoEngine::VulkanGPUBuffer vertexBuffer;
    bool vInit = vertexBuffer.Initialize(device, physicalDevice, 128, NeoEngine::VulkanBufferType::VertexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    TEST_CHECK(vInit, "VertexBuffer initialization failed");

    NeoEngine::VulkanGPUBuffer indexBuffer;
    bool iInit = indexBuffer.Initialize(device, physicalDevice, 64, NeoEngine::VulkanBufferType::IndexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    TEST_CHECK(iInit, "IndexBuffer initialization failed");

    // 3. Initialize RenderCommandRecorder
    NeoEngine::VulkanRenderCommandRecorder recorder;
    bool rInit = recorder.Initialize(device, commandPool);
    TEST_CHECK(rInit, "Recorder initialization failed");
    TEST_CHECK(recorder.IsValid(), "Recorder IsValid check failed");
    TEST_CHECK(recorder.GetCommandBuffer() != VK_NULL_HANDLE, "GetCommandBuffer returned NULL");

    // 4. Record Command Buffer Sequence
    bool recBegin = recorder.BeginRecording();
    TEST_CHECK(recBegin, "BeginRecording failed");
    TEST_CHECK(recorder.IsRecording(), "IsRecording check failed");

    recorder.SetViewportAndScissor(800, 600);
    recorder.BindVertexBuffer(vertexBuffer.GetBuffer());
    recorder.BindIndexBuffer(indexBuffer.GetBuffer());
    recorder.DrawIndexed(6);

    bool recEnd = recorder.EndRecording();
    TEST_CHECK(recEnd, "EndRecording failed");
    TEST_CHECK(!recorder.IsRecording(), "IsRecording should be false after EndRecording");

    // 5. Test Move Semantics
    NeoEngine::VulkanRenderCommandRecorder movedRecorder = std::move(recorder);
    TEST_CHECK(!recorder.IsValid(), "Moved-from recorder should be invalid");
    TEST_CHECK(movedRecorder.IsValid(), "Moved-to recorder should be valid");

    // RAII Cleanup
    vertexBuffer.Destroy();
    indexBuffer.Destroy();
    movedRecorder.Destroy();
    vkDestroyCommandPool(device, commandPool, nullptr);
    context.Reset();

    std::cout << "[Smoke Test] vulkan_render_command_recorder_smoke passed successfully!\n";
    return 0;
}
