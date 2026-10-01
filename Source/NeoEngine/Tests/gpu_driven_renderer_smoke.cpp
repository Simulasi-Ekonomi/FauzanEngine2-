#include "Renderer/GPUDrivenRenderer.h"

#include <cstdint>
#include <vector>

#define REQUIRE(...) do { if (!(__VA_ARGS__)) return 1; } while (false)

int main() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FauzanEngine2 R3 GPU indirect smoke";
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "FauzanEngine2";
    app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &app;

    VkInstance instance = VK_NULL_HANDLE;
    REQUIRE(vkCreateInstance(&instanceInfo, nullptr, &instance) == VK_SUCCESS);

    uint32_t physicalCount = 0;
    REQUIRE(vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr) == VK_SUCCESS);
    REQUIRE(physicalCount > 0);
    std::vector<VkPhysicalDevice> physicalDevices(physicalCount);
    REQUIRE(vkEnumeratePhysicalDevices(instance, &physicalCount, physicalDevices.data()) == VK_SUCCESS);

    VkPhysicalDevice physical = physicalDevices.front();
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, families.data());

    uint32_t graphicsFamily = UINT32_MAX;
    for (uint32_t i = 0; i < familyCount; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
            graphicsFamily = i;
            break;
        }
    }
    REQUIRE(graphicsFamily != UINT32_MAX);

    float priority = 1.0F;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = graphicsFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;

    VkDevice device = VK_NULL_HANDLE;
    REQUIRE(vkCreateDevice(physical, &deviceInfo, nullptr, &device) == VK_SUCCESS);

    GPUDrivenRenderer indirect;
    REQUIRE(indirect.Initialize(device, physical, 128));
    REQUIRE(indirect.IsInitialized());
    REQUIRE(indirect.HasGpuBuffer());
    REQUIRE(indirect.Capacity() > 0);

    REQUIRE(indirect.TrySubmitDraw({36, 1, 0, 0, 0}));
    REQUIRE(indirect.TrySubmitDraw({36, 8, 36, 12, 1}));
    REQUIRE(indirect.TrySubmitDraw({6, 64, 72, 20, 9}));
    REQUIRE(indirect.PendingDrawCount() == 3);

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool = VK_NULL_HANDLE;
    REQUIRE(vkCreateCommandPool(device, &poolInfo, nullptr, &pool) == VK_SUCCESS);

    VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocation.commandPool = pool;
    allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    REQUIRE(vkAllocateCommandBuffers(device, &allocation, &commandBuffer) == VK_SUCCESS);

    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    REQUIRE(vkBeginCommandBuffer(commandBuffer, &begin) == VK_SUCCESS);
    REQUIRE(indirect.Execute(commandBuffer));
    REQUIRE(indirect.PendingDrawCount() == 0);
    REQUIRE(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);

    indirect.Destroy();
    vkDestroyCommandPool(device, pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    return 0;
}
