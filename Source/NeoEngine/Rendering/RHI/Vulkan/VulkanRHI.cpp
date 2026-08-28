#include "VulkanRHI.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace NeoEngine {
namespace {
VkSurfaceFormatKHR ChooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const auto& f : formats) if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f;
    return formats.front();
}
VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (const auto mode : modes) if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& caps, uint32_t w, uint32_t h) {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) return caps.currentExtent;
    return {std::clamp(w, caps.minImageExtent.width, caps.maxImageExtent.width), std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height)};
}
}
VulkanRHI& VulkanRHI::Get() { static VulkanRHI instance; return instance; }

bool VulkanRHI::Init(void* nativeWindow, int w, int h, const char* appName) {
    if (m_Initialized) return true;
    if (nativeWindow == nullptr || w <= 0 || h <= 0 || appName == nullptr || std::strlen(appName) == 0) return false;
    auto* window = static_cast<SDL_Window*>(nativeWindow);
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0 && SDL_Init(SDL_INIT_VIDEO) != 0) return false;
    unsigned extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr) != SDL_TRUE || extensionCount == 0) return false;
    std::vector<const char*> extensions(extensionCount);
    if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data()) != SDL_TRUE) return false;
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO}; app.pApplicationName = appName; app.applicationVersion = VK_MAKE_VERSION(1, 0, 0); app.pEngineName = "FauzanEngine"; app.engineVersion = VK_MAKE_VERSION(1, 0, 0); app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; instanceInfo.pApplicationInfo = &app; instanceInfo.enabledExtensionCount = extensionCount; instanceInfo.ppEnabledExtensionNames = extensions.data();
    if (vkCreateInstance(&instanceInfo, nullptr, &m_Instance) != VK_SUCCESS) { Shutdown(); return false; }
    if (SDL_Vulkan_CreateSurface(window, m_Instance, &m_Surface) != SDL_TRUE) { Shutdown(); return false; }
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) { Shutdown(); return false; }
    std::vector<VkPhysicalDevice> devices(deviceCount); if (vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data()) != VK_SUCCESS) { Shutdown(); return false; }
    for (VkPhysicalDevice candidate : devices) {
        uint32_t familyCount = 0; vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr); std::vector<VkQueueFamilyProperties> families(familyCount); vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
        for (uint32_t family = 0; family < familyCount; ++family) {
            VkBool32 present = VK_FALSE;
            if ((families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || families[family].queueCount == 0 || vkGetPhysicalDeviceSurfaceSupportKHR(candidate, family, m_Surface, &present) != VK_SUCCESS || present != VK_TRUE) continue;
            uint32_t formatCount = 0, modeCount = 0;
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, m_Surface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0 || vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, m_Surface, &modeCount, nullptr) != VK_SUCCESS || modeCount == 0) continue;
            std::vector<VkSurfaceFormatKHR> formats(formatCount); std::vector<VkPresentModeKHR> modes(modeCount); vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, m_Surface, &formatCount, formats.data()); vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, m_Surface, &modeCount, modes.data());
            const auto surfaceFormat = ChooseFormat(formats); const auto presentMode = ChoosePresentMode(modes); VkSurfaceCapabilitiesKHR caps{}; if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(candidate, m_Surface, &caps) != VK_SUCCESS) continue;
            const VkExtent2D extent = ChooseExtent(caps, static_cast<uint32_t>(w), static_cast<uint32_t>(h)); uint32_t imageCount = caps.minImageCount + 1; if (caps.maxImageCount != 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;
            const char* swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME; const float priority = 1.0F;
            VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}; queueInfo.queueFamilyIndex = family; queueInfo.queueCount = 1; queueInfo.pQueuePriorities = &priority;
            VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO}; deviceInfo.queueCreateInfoCount = 1; deviceInfo.pQueueCreateInfos = &queueInfo; deviceInfo.enabledExtensionCount = 1; deviceInfo.ppEnabledExtensionNames = &swapchainExtension;
            if (vkCreateDevice(candidate, &deviceInfo, nullptr, &m_Device) != VK_SUCCESS) continue;
            m_GPU = candidate; m_QueueFamily = family; vkGetDeviceQueue(m_Device, family, 0, &m_GraphicsQueue);
            VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR}; swapInfo.surface = m_Surface; swapInfo.minImageCount = imageCount; swapInfo.imageFormat = surfaceFormat.format; swapInfo.imageColorSpace = surfaceFormat.colorSpace; swapInfo.imageExtent = extent; swapInfo.imageArrayLayers = 1; swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; swapInfo.preTransform = caps.currentTransform; swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; swapInfo.presentMode = presentMode; swapInfo.clipped = VK_TRUE;
            if (vkCreateSwapchainKHR(m_Device, &swapInfo, nullptr, &m_Swapchain) != VK_SUCCESS) { Shutdown(); return false; }
            m_SwapchainFormat = surfaceFormat.format; m_Width = static_cast<int>(extent.width); m_Height = static_cast<int>(extent.height);
            if (!CreateSwapchainResources(extent.width, extent.height)) { Shutdown(); return false; }
            m_Initialized = true; return true;
        }
    }
    Shutdown(); return false;
}

bool VulkanRHI::CreateSwapchainResources(uint32_t, uint32_t) {
    uint32_t count = 0; if (vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &count, nullptr) != VK_SUCCESS || count == 0) return false; m_SwapchainImages.resize(count); if (vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &count, m_SwapchainImages.data()) != VK_SUCCESS) return false;
    m_SwapchainViews.resize(count); m_Framebuffers.resize(count); for (uint32_t i = 0; i < count; ++i) { VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO}; view.image = m_SwapchainImages[i]; view.viewType = VK_IMAGE_VIEW_TYPE_2D; view.format = m_SwapchainFormat; view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; view.subresourceRange.levelCount = 1; view.subresourceRange.layerCount = 1; if (vkCreateImageView(m_Device, &view, nullptr, &m_SwapchainViews[i]) != VK_SUCCESS) return false; }
    VkAttachmentDescription attachment{}; attachment.format = m_SwapchainFormat; attachment.samples = VK_SAMPLE_COUNT_1_BIT; attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; VkSubpassDescription subpass{}; subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; subpass.colorAttachmentCount = 1; subpass.pColorAttachments = &color;
    VkSubpassDependency dependency{}; dependency.srcSubpass = VK_SUBPASS_EXTERNAL; dependency.dstSubpass = 0; dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO}; pass.attachmentCount = 1; pass.pAttachments = &attachment; pass.subpassCount = 1; pass.pSubpasses = &subpass; pass.dependencyCount = 1; pass.pDependencies = &dependency; if (vkCreateRenderPass(m_Device, &pass, nullptr, &m_RenderPass) != VK_SUCCESS) return false;
    for (uint32_t i = 0; i < count; ++i) { VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO}; fb.renderPass = m_RenderPass; fb.attachmentCount = 1; fb.pAttachments = &m_SwapchainViews[i]; fb.width = static_cast<uint32_t>(m_Width); fb.height = static_cast<uint32_t>(m_Height); fb.layers = 1; if (vkCreateFramebuffer(m_Device, &fb, nullptr, &m_Framebuffers[i]) != VK_SUCCESS) return false; }
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; pool.queueFamilyIndex = m_QueueFamily; pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; if (vkCreateCommandPool(m_Device, &pool, nullptr, &m_CommandPool) != VK_SUCCESS) return false;
    VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO}; alloc.commandPool = m_CommandPool; alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; alloc.commandBufferCount = 1; if (vkAllocateCommandBuffers(m_Device, &alloc, &m_CommandBuffer) != VK_SUCCESS) return false;
    VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}; if (vkCreateSemaphore(m_Device, &semaphore, nullptr, &m_ImageAvailable) != VK_SUCCESS || vkCreateSemaphore(m_Device, &semaphore, nullptr, &m_RenderFinished) != VK_SUCCESS) return false;
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}; fence.flags = VK_FENCE_CREATE_SIGNALED_BIT; return vkCreateFence(m_Device, &fence, nullptr, &m_InFlight) == VK_SUCCESS;
}

void VulkanRHI::DestroySwapchainResources() {
    if (m_Device == VK_NULL_HANDLE) return; if (m_InFlight != VK_NULL_HANDLE) vkDestroyFence(m_Device, m_InFlight, nullptr); if (m_RenderFinished != VK_NULL_HANDLE) vkDestroySemaphore(m_Device, m_RenderFinished, nullptr); if (m_ImageAvailable != VK_NULL_HANDLE) vkDestroySemaphore(m_Device, m_ImageAvailable, nullptr); if (m_CommandPool != VK_NULL_HANDLE) vkDestroyCommandPool(m_Device, m_CommandPool, nullptr); for (auto fb : m_Framebuffers) if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(m_Device, fb, nullptr); if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Device, m_RenderPass, nullptr); for (auto view : m_SwapchainViews) if (view != VK_NULL_HANDLE) vkDestroyImageView(m_Device, view, nullptr); if (m_Swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr); m_Framebuffers.clear(); m_SwapchainViews.clear(); m_SwapchainImages.clear(); m_InFlight = VK_NULL_HANDLE; m_RenderFinished = VK_NULL_HANDLE; m_ImageAvailable = VK_NULL_HANDLE; m_CommandPool = VK_NULL_HANDLE; m_CommandBuffer = VK_NULL_HANDLE; m_RenderPass = VK_NULL_HANDLE; m_Swapchain = VK_NULL_HANDLE;
}
void VulkanRHI::Shutdown() { if (m_Device != VK_NULL_HANDLE) vkDeviceWaitIdle(m_Device); DestroySwapchainResources(); if (m_Device != VK_NULL_HANDLE) vkDestroyDevice(m_Device, nullptr); if (m_Surface != VK_NULL_HANDLE && m_Instance != VK_NULL_HANDLE) vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr); if (m_Instance != VK_NULL_HANDLE) vkDestroyInstance(m_Instance, nullptr); m_Instance = VK_NULL_HANDLE; m_GPU = VK_NULL_HANDLE; m_Device = VK_NULL_HANDLE; m_GraphicsQueue = VK_NULL_HANDLE; m_Surface = VK_NULL_HANDLE; m_SwapchainFormat = VK_FORMAT_UNDEFINED; m_QueueFamily = UINT32_MAX; m_ImageIndex = UINT32_MAX; m_Width = 0; m_Height = 0; m_FrameActive = false; m_ImageAcquired = false; m_FrameSubmitted = false; m_Initialized = false; }
void VulkanRHI::BeginFrame() { if (!m_Initialized || m_FrameActive || m_Swapchain == VK_NULL_HANDLE) return; if (vkWaitForFences(m_Device, 1, &m_InFlight, VK_TRUE, UINT64_MAX) != VK_SUCCESS) return; if (vkResetFences(m_Device, 1, &m_InFlight) != VK_SUCCESS) return; const VkResult acquired = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailable, VK_NULL_HANDLE, &m_ImageIndex); if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR) return; if (vkResetCommandBuffer(m_CommandBuffer, 0) != VK_SUCCESS) return; VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; if (vkBeginCommandBuffer(m_CommandBuffer, &begin) != VK_SUCCESS) return; VkClearValue clear{}; clear.color = {{0.02F, 0.02F, 0.025F, 1.0F}}; VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO}; pass.renderPass = m_RenderPass; pass.framebuffer = m_Framebuffers[m_ImageIndex]; pass.renderArea.extent = {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)}; pass.clearValueCount = 1; pass.pClearValues = &clear; vkCmdBeginRenderPass(m_CommandBuffer, &pass, VK_SUBPASS_CONTENTS_INLINE); m_ImageAcquired = true; m_FrameActive = true; m_FrameSubmitted = false; }
void VulkanRHI::EndFrame() { if (!m_FrameActive || !m_ImageAcquired) return; vkCmdEndRenderPass(m_CommandBuffer); if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS) { m_FrameActive = false; m_ImageAcquired = false; m_FrameSubmitted = false; return; } VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.waitSemaphoreCount = 1; submit.pWaitSemaphores = &m_ImageAvailable; submit.pWaitDstStageMask = &waitStage; submit.commandBufferCount = 1; submit.pCommandBuffers = &m_CommandBuffer; submit.signalSemaphoreCount = 1; submit.pSignalSemaphores = &m_RenderFinished; if (vkQueueSubmit(m_GraphicsQueue, 1, &submit, m_InFlight) != VK_SUCCESS) { m_FrameActive = false; m_ImageAcquired = false; m_FrameSubmitted = false; return; } m_FrameActive = false; m_FrameSubmitted = true; }
void VulkanRHI::Present() { if (m_Swapchain == VK_NULL_HANDLE || m_ImageIndex == UINT32_MAX || !m_FrameSubmitted) return; VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR}; present.waitSemaphoreCount = 1; present.pWaitSemaphores = &m_RenderFinished; present.swapchainCount = 1; present.pSwapchains = &m_Swapchain; present.pImageIndices = &m_ImageIndex; vkQueuePresentKHR(m_GraphicsQueue, &present); m_ImageAcquired = false; m_FrameSubmitted = false; m_ImageIndex = UINT32_MAX; }
} // namespace NeoEngine
