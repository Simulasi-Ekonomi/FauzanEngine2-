#include "VulkanContext.h"

#include <vector>

namespace NeoEngine {

VulkanContext::~VulkanContext() {
    Reset();
}

bool VulkanContext::Initialize() {
    Reset();

    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = "NeoEngine";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "NeoEngine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &applicationInfo;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance_) != VK_SUCCESS) {
        return false;
    }

    if (vkEnumeratePhysicalDevices(instance_, &physicalDeviceCount_, nullptr) != VK_SUCCESS || physicalDeviceCount_ == 0) {
        Reset();
        return false;
    }

    std::vector<VkPhysicalDevice> devices(physicalDeviceCount_);
    if (vkEnumeratePhysicalDevices(instance_, &physicalDeviceCount_, devices.data()) != VK_SUCCESS) {
        Reset();
        return false;
    }

    for (const VkPhysicalDevice candidate : devices) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
        if (familyCount == 0) {
            continue;
        }

        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
        for (uint32_t index = 0; index < familyCount; ++index) {
            if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || families[index].queueCount == 0) {
                continue;
            }

            const float priority = 1.0F;
            VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            queueInfo.queueFamilyIndex = index;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &priority;

            VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            deviceInfo.queueCreateInfoCount = 1;
            deviceInfo.pQueueCreateInfos = &queueInfo;

            VkDevice candidateDevice = VK_NULL_HANDLE;
            if (vkCreateDevice(candidate, &deviceInfo, nullptr, &candidateDevice) != VK_SUCCESS) {
                continue;
            }

            physicalDevice_ = candidate;
            device_ = candidateDevice;
            graphicsQueueFamily_ = index;
            vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
            return graphicsQueue_ != VK_NULL_HANDLE;
        }
    }

    Reset();
    return false;
}

void VulkanContext::Reset() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
    instance_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    graphicsQueue_ = VK_NULL_HANDLE;
    graphicsQueueFamily_ = UINT32_MAX;
}

} // namespace NeoEngine
