#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanSyncPrimitives.h"
#include <iostream>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting vulkan_sync_primitives_smoke...\n";

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

    // 1. Test VulkanSemaphore
    NeoEngine::VulkanSemaphore semA;
    bool semInit = semA.Initialize(device);
    TEST_CHECK(semInit, "Semaphore A initialization failed");
    TEST_CHECK(semA.IsValid(), "Semaphore A IsValid check failed");
    TEST_CHECK(semA.GetSemaphore() != VK_NULL_HANDLE, "GetSemaphore returned NULL");

    NeoEngine::VulkanSemaphore semB = std::move(semA);
    TEST_CHECK(!semA.IsValid(), "Moved-from semaphore should be invalid");
    TEST_CHECK(semB.IsValid(), "Moved-to semaphore should be valid");

    // 2. Test VulkanFence
    NeoEngine::VulkanFence fenceSignaled;
    bool fenceInit = fenceSignaled.Initialize(device, true);
    TEST_CHECK(fenceInit, "Signaled fence initialization failed");
    TEST_CHECK(fenceSignaled.IsValid(), "Fence IsValid check failed");
    TEST_CHECK(fenceSignaled.Wait(1000000), "Wait on signaled fence should succeed");

    bool resetOk = fenceSignaled.Reset();
    TEST_CHECK(resetOk, "Reset fence failed");

    NeoEngine::VulkanFence fenceMoved = std::move(fenceSignaled);
    TEST_CHECK(!fenceSignaled.IsValid(), "Moved-from fence should be invalid");
    TEST_CHECK(fenceMoved.IsValid(), "Moved-to fence should be valid");

    // RAII Cleanup
    semB.Destroy();
    fenceMoved.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_sync_primitives_smoke passed successfully!\n";
    return 0;
}
