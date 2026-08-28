#include "VulkanRHI.h"
#include <vector>

void VulkanRHI::Init() {
    if (initialized_) return;
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FauzanEngine";
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "FauzanEngine";
    app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &app;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance_) != VK_SUCCESS) return;

    uint32_t count = 0;
    if (vkEnumeratePhysicalDevices(instance_, &count, nullptr) != VK_SUCCESS || count == 0) {
        Shutdown();
        return;
    }
    std::vector<VkPhysicalDevice> devices(count);
    if (vkEnumeratePhysicalDevices(instance_, &count, devices.data()) != VK_SUCCESS) {
        Shutdown();
        return;
    }
    for (VkPhysicalDevice candidate : devices) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
        for (uint32_t family = 0; family < familyCount; ++family) {
            if ((families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || families[family].queueCount == 0) continue;
            const float priority = 1.0F;
            VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            queueInfo.queueFamilyIndex = family;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &priority;
            VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            deviceInfo.queueCreateInfoCount = 1;
            deviceInfo.pQueueCreateInfos = &queueInfo;
            if (vkCreateDevice(candidate, &deviceInfo, nullptr, &device_) != VK_SUCCESS) continue;
            physicalDevice_ = candidate;
            vkGetDeviceQueue(device_, family, 0, &graphicsQueue_);
            if (graphicsQueue_ == VK_NULL_HANDLE) {
                Shutdown();
                return;
            }
            initialized_ = true;
            return;
        }
    }
    Shutdown();
}

void VulkanRHI::BeginFrame() {
    if (initialized_) frameActive_ = true;
}

void VulkanRHI::EndFrame() {
    frameActive_ = false;
}

void VulkanRHI::Shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
    }
    if (instance_ != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    graphicsQueue_ = VK_NULL_HANDLE;
    initialized_ = false;
    frameActive_ = false;
}
