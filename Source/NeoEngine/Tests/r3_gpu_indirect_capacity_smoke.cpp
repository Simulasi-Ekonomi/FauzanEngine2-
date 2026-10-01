#include "Renderer/GPUDrivenRenderer.h"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

#define REQUIRE(...) do { if (!(__VA_ARGS__)) return 1; } while (false)

int main() {
    constexpr std::size_t kCommandCount = GPUDrivenRenderer::MaxCommands;

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FauzanEngine2 R3 indirect capacity smoke";
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

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical, &properties);
    REQUIRE(properties.limits.maxDrawIndirectCount >= kCommandCount);

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
    REQUIRE(indirect.Initialize(device, physical, kCommandCount));
    REQUIRE(indirect.Capacity() == kCommandCount);

    const GPUIndirectCommand command{3, 1, 0, 0, 0};
    for (std::size_t i = 0; i < kCommandCount; ++i) {
        REQUIRE(indirect.TrySubmitDraw(command));
    }
    REQUIRE(indirect.PendingDrawCount() == kCommandCount);
    REQUIRE(!indirect.TrySubmitDraw(command));

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool = VK_NULL_HANDLE;
    REQUIRE(vkCreateCommandPool(device, &poolInfo, nullptr, &pool) == VK_SUCCESS);

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    REQUIRE(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) == VK_SUCCESS);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    REQUIRE(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS);
    REQUIRE(indirect.Execute(commandBuffer));
    REQUIRE(indirect.PendingDrawCount() == 0);
    REQUIRE(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);

    indirect.Destroy();
    vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
    vkDestroyCommandPool(device, pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    return 0;
}
