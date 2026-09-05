#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanSwapchainManager.h"
#include <iostream>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting vulkan_swapchain_manager_smoke...\n";

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

    // Since headless runners lack window surface creation, we test QuerySwapchainSupport and Move Semantics on manager
    VkSurfaceKHR dummySurface = VK_NULL_HANDLE;
    NeoEngine::SwapchainSupportDetails support = NeoEngine::VulkanSwapchainManager::QuerySwapchainSupport(physicalDevice, dummySurface);
    TEST_CHECK(support.formats.empty(), "QuerySwapchainSupport on NULL surface should be empty");

    NeoEngine::VulkanSwapchainManager manager;
    TEST_CHECK(!manager.IsValid(), "Default manager should be invalid");

    // Test Move Semantics
    NeoEngine::VulkanSwapchainManager movedManager = std::move(manager);
    TEST_CHECK(!movedManager.IsValid(), "Moved default manager should be invalid");

    // RAII Cleanup
    movedManager.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_swapchain_manager_smoke passed successfully!\n";
    return 0;
}
