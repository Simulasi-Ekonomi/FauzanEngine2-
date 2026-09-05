#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanGPUTexture.h"
#include <iostream>
#include <vector>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting vulkan_gpu_texture_smoke...\n";

    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cout << "[INFO] VulkanContext failed to initialize (likely headless environment). Skipping hardware test gracefully.\n";
        return 0;
    }

    VkDevice device = context.Device();
    VkPhysicalDevice physicalDevice = context.PhysicalDevice();
    VkQueue graphicsQueue = context.GraphicsQueue();
    uint32_t queueFamily = context.GraphicsQueueFamily();

    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        std::cout << "[INFO] No valid Vulkan physical or logical device available. Skipping test gracefully.\n";
        return 0;
    }

    // 1. Create 2x2 Texture
    uint32_t width = 2;
    uint32_t height = 2;
    NeoEngine::VulkanGPUTexture texture;
    bool init = texture.Initialize(device, physicalDevice, width, height, VK_FORMAT_R8G8B8A8_UNORM);
    TEST_CHECK(init, "Texture initialization failed");
    TEST_CHECK(texture.IsValid(), "Texture IsValid check failed");
    TEST_CHECK(texture.GetWidth() == width, "Texture width mismatch");
    TEST_CHECK(texture.GetHeight() == height, "Texture height mismatch");

    // 2. Upload RGBA8 Pixels (4 pixels x 4 bytes = 16 bytes)
    std::vector<uint8_t> pixels = {
        255, 0, 0, 255,   // Red
        0, 255, 0, 255,   // Green
        0, 0, 255, 255,   // Blue
        255, 255, 0, 255  // Yellow
    };

    if (graphicsQueue != VK_NULL_HANDLE) {
        bool upload = texture.UploadPixels(graphicsQueue, queueFamily, physicalDevice, pixels.data(), pixels.size());
        TEST_CHECK(upload, "Texture pixel upload failed");
    }

    // 3. Create Sampler
    bool samplerCreated = texture.CreateSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    TEST_CHECK(samplerCreated, "Sampler creation failed");
    TEST_CHECK(texture.GetSampler() != VK_NULL_HANDLE, "GetSampler returned NULL");

    // 4. Test Move Semantics
    NeoEngine::VulkanGPUTexture movedTexture = std::move(texture);
    TEST_CHECK(!texture.IsValid(), "Moved-from texture should be invalid");
    TEST_CHECK(movedTexture.IsValid(), "Moved-to texture should be valid");
    TEST_CHECK(movedTexture.GetSampler() != VK_NULL_HANDLE, "Moved-to sampler should be valid");

    // RAII Cleanup
    movedTexture.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_gpu_texture_smoke passed successfully!\n";
    return 0;
}
