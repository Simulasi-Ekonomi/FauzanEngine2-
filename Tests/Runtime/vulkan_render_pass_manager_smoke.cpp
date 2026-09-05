#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanGPUTexture.h"
#include "Runtime/VulkanRenderPassManager.h"
#include <iostream>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting vulkan_render_pass_manager_smoke...\n";

    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cout << "[INFO] VulkanContext failed to initialize (likely headless environment). Skipping hardware test gracefully.\n";
        return 0;
    }

    VkDevice device = context.Device();
    VkPhysicalDevice physicalDevice = context.PhysicalDevice();

    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        std::cout << "[INFO] No valid Vulkan physical or logical device available. Skipping test gracefully.\n";
        return 0;
    }

    // 1. Initialize RenderPassManager
    NeoEngine::RenderPassConfig config{};
    config.colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    config.depthFormat = VK_FORMAT_D32_SFLOAT;
    config.enableDepth = true;

    NeoEngine::VulkanRenderPassManager manager;
    bool init = manager.Initialize(device, config);
    TEST_CHECK(init, "RenderPassManager initialization failed");
    TEST_CHECK(manager.IsValid(), "RenderPassManager IsValid check failed");
    TEST_CHECK(manager.GetRenderPass() != VK_NULL_HANDLE, "GetRenderPass returned NULL");

    // 2. Create Attachment Textures (Color & Depth)
    uint32_t width = 800;
    uint32_t height = 600;

    NeoEngine::VulkanGPUTexture colorTex;
    bool cInit = colorTex.Initialize(device, physicalDevice, width, height, VK_FORMAT_R8G8B8A8_UNORM);
    TEST_CHECK(cInit, "Color texture initialization failed");

    // 3. Create Framebuffer
    bool fbInit = manager.CreateFramebuffer(colorTex.GetImageView(), VK_NULL_HANDLE, width, height);
    TEST_CHECK(fbInit, "Framebuffer creation failed");
    TEST_CHECK(manager.GetFramebuffer() != VK_NULL_HANDLE, "GetFramebuffer returned NULL");
    TEST_CHECK(manager.GetWidth() == width, "Framebuffer width mismatch");
    TEST_CHECK(manager.GetHeight() == height, "Framebuffer height mismatch");

    // 4. Test Move Semantics
    NeoEngine::VulkanRenderPassManager movedManager = std::move(manager);
    TEST_CHECK(!manager.IsValid(), "Moved-from manager should be invalid");
    TEST_CHECK(movedManager.IsValid(), "Moved-to manager should be valid");
    TEST_CHECK(movedManager.GetFramebuffer() != VK_NULL_HANDLE, "Moved-to framebuffer should be valid");

    // RAII Cleanup
    colorTex.Destroy();
    movedManager.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_render_pass_manager_smoke passed successfully!\n";
    return 0;
}
