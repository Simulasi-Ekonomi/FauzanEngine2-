#include "VulkanRHI.h"
#include <cstring>
#include <vector>

namespace NeoEngine {
VulkanRHI& VulkanRHI::Get() {
    static VulkanRHI instance;
    return instance;
}

bool VulkanRHI::Init(void* nativeWindow, int w, int h, const char* appName) {
    (void)nativeWindow;
    if (m_Initialized) return true;
    if (w <= 0 || h <= 0 || appName == nullptr || std::strlen(appName) == 0) return false;

    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = appName;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "FauzanEngine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &applicationInfo;
    if (vkCreateInstance(&instanceInfo, nullptr, &m_Instance) != VK_SUCCESS) return false;

    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
        Shutdown();
        return false;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data()) != VK_SUCCESS) {
        Shutdown();
        return false;
    }
    for (const VkPhysicalDevice candidate : devices) {
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
            if (vkCreateDevice(candidate, &deviceInfo, nullptr, &m_Device) != VK_SUCCESS) continue;
            m_GPU = candidate;
            vkGetDeviceQueue(m_Device, family, 0, &m_GraphicsQueue);
            if (m_GraphicsQueue == VK_NULL_HANDLE) {
                Shutdown();
                return false;
            }
            m_Width = w;
            m_Height = h;
            m_Initialized = true;
            m_FrameActive = false;
            return true;
        }
    }
    Shutdown();
    return false;
}

void VulkanRHI::Shutdown() {
    if (m_Device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_Device);
        vkDestroyDevice(m_Device, nullptr);
    }
    if (m_Instance != VK_NULL_HANDLE) vkDestroyInstance(m_Instance, nullptr);
    m_Instance = VK_NULL_HANDLE;
    m_GPU = VK_NULL_HANDLE;
    m_Device = VK_NULL_HANDLE;
    m_GraphicsQueue = VK_NULL_HANDLE;
    m_Swapchain = VK_NULL_HANDLE;
    m_SwapchainImages.clear();
    m_SwapchainFormat = VK_FORMAT_UNDEFINED;
    m_Width = 0;
    m_Height = 0;
    m_FrameActive = false;
    m_Initialized = false;
}

void VulkanRHI::BeginFrame() {
    if (m_Initialized && !m_FrameActive) m_FrameActive = true;
}

void VulkanRHI::EndFrame() {
    if (m_FrameActive) m_FrameActive = false;
}

void VulkanRHI::Present() {
    if (m_FrameActive) EndFrame();
}
} // namespace NeoEngine
