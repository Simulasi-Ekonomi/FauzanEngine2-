#include "VulkanPresentProbe.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <limits>
#include <vector>

namespace NeoEngine {
namespace {

struct Resources {
    SDL_Window* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamily = UINT32_MAX;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImageView> views;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commands;
    VkSemaphore acquired = VK_NULL_HANDLE;
    VkSemaphore rendered = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    ~Resources() {
        if (device != VK_NULL_HANDLE) vkDeviceWaitIdle(device);
        if (fence != VK_NULL_HANDLE) vkDestroyFence(device, fence, nullptr);
        if (rendered != VK_NULL_HANDLE) vkDestroySemaphore(device, rendered, nullptr);
        if (acquired != VK_NULL_HANDLE) vkDestroySemaphore(device, acquired, nullptr);
        if (pool != VK_NULL_HANDLE) vkDestroyCommandPool(device, pool, nullptr);
        for (VkFramebuffer framebuffer : framebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(device, renderPass, nullptr);
        for (VkImageView view : views) vkDestroyImageView(device, view, nullptr);
        if (swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, swapchain, nullptr);
        if (device != VK_NULL_HANDLE) vkDestroyDevice(device, nullptr);
        if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, nullptr);
        if (window != nullptr) SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
};

bool ChooseDevice(Resources& resources) {
    uint32_t count = 0;
    if (vkEnumeratePhysicalDevices(resources.instance, &count, nullptr) != VK_SUCCESS || count == 0) return false;
    std::vector<VkPhysicalDevice> devices(count);
    if (vkEnumeratePhysicalDevices(resources.instance, &count, devices.data()) != VK_SUCCESS) return false;
    for (VkPhysicalDevice candidate : devices) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
        for (uint32_t family = 0; family < familyCount; ++family) {
            VkBool32 supportsPresent = VK_FALSE;
            if ((families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || families[family].queueCount == 0 ||
                vkGetPhysicalDeviceSurfaceSupportKHR(candidate, family, resources.surface, &supportsPresent) != VK_SUCCESS || supportsPresent != VK_TRUE) {
                continue;
            }
            const float priority = 1.0F;
            VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            queueInfo.queueFamilyIndex = family;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &priority;
            const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            deviceInfo.queueCreateInfoCount = 1;
            deviceInfo.pQueueCreateInfos = &queueInfo;
            deviceInfo.enabledExtensionCount = 1;
            deviceInfo.ppEnabledExtensionNames = &extension;
            VkDevice device = VK_NULL_HANDLE;
            if (vkCreateDevice(candidate, &deviceInfo, nullptr, &device) != VK_SUCCESS) continue;
            resources.physical = candidate;
            resources.device = device;
            resources.queueFamily = family;
            vkGetDeviceQueue(device, family, 0, &resources.queue);
            return resources.queue != VK_NULL_HANDLE;
        }
    }
    return false;
}

VkSurfaceFormatKHR ChooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    const auto preferred = std::find_if(formats.begin(), formats.end(), [](const VkSurfaceFormatKHR& format) {
        return format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    return preferred != formats.end() ? *preferred : formats.front();
}

VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;
    return {
        std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
}

} // namespace

VulkanPresentProbeResult VulkanPresentProbe::PresentHiddenFrame(uint32_t width, uint32_t height) {
    VulkanPresentProbeResult result{};
    if (width == 0 || height == 0 || width > 2048 || height > 2048) return result;
    Resources resources{};
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return result;
    resources.window = SDL_CreateWindow("NeoEngine Present Probe", static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    if (resources.window == nullptr) return result;
    result.windowCreated = true;
    unsigned extensionCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (sdlExtensions == nullptr || extensionCount == 0) return result;
    std::vector<const char*> extensions(sdlExtensions, sdlExtensions + extensionCount);
    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = "NeoEnginePresentProbe";
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &applicationInfo;
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    if (vkCreateInstance(&instanceInfo, nullptr, &resources.instance) != VK_SUCCESS || !SDL_Vulkan_CreateSurface(resources.window, resources.instance, nullptr, &resources.surface)) return result;
    result.surfaceCreated = true;
    if (!ChooseDevice(resources)) return result;
    result.deviceCreated = true;
    VkSurfaceCapabilitiesKHR capabilities{};
    uint32_t formatCount = 0;
    uint32_t presentModeCount = 0;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(resources.physical, resources.surface, &capabilities) != VK_SUCCESS ||
        vkGetPhysicalDeviceSurfaceFormatsKHR(resources.physical, resources.surface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0 ||
        vkGetPhysicalDeviceSurfacePresentModesKHR(resources.physical, resources.surface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) return result;
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(resources.physical, resources.surface, &formatCount, formats.data()) != VK_SUCCESS) return result;
    const VkSurfaceFormatKHR surfaceFormat = ChooseFormat(formats);
    const VkExtent2D extent = ChooseExtent(capabilities, width, height);
    uint32_t imageCount = capabilities.minImageCount + 1U;
    if (capabilities.maxImageCount != 0 && imageCount > capabilities.maxImageCount) imageCount = capabilities.maxImageCount;
    VkSwapchainCreateInfoKHR swapchainInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = resources.surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(resources.device, &swapchainInfo, nullptr, &resources.swapchain) != VK_SUCCESS) return result;
    uint32_t swapchainImageCount = 0;
    if (vkGetSwapchainImagesKHR(resources.device, resources.swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS || swapchainImageCount == 0) return result;
    std::vector<VkImage> images(swapchainImageCount);
    if (vkGetSwapchainImagesKHR(resources.device, resources.swapchain, &swapchainImageCount, images.data()) != VK_SUCCESS) return result;
    result.swapchainCreated = true;
    result.imageCount = swapchainImageCount;
    resources.views.reserve(images.size());
    for (VkImage image : images) {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = surfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        VkImageView view = VK_NULL_HANDLE;
        if (vkCreateImageView(resources.device, &viewInfo, nullptr, &view) != VK_SUCCESS) return result;
        resources.views.push_back(view);
    }
    VkAttachmentDescription attachment{};
    attachment.format = surfaceFormat.format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference reference{};
    reference.attachment = 0;
    reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    passInfo.attachmentCount = 1;
    passInfo.pAttachments = &attachment;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpass;
    if (vkCreateRenderPass(resources.device, &passInfo, nullptr, &resources.renderPass) != VK_SUCCESS) return result;
    resources.framebuffers.reserve(resources.views.size());
    for (VkImageView view : resources.views) {
        VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = resources.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &view;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (vkCreateFramebuffer(resources.device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) return result;
        resources.framebuffers.push_back(framebuffer);
    }
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = resources.queueFamily;
    if (vkCreateCommandPool(resources.device, &poolInfo, nullptr, &resources.pool) != VK_SUCCESS) return result;
    resources.commands.resize(images.size());
    VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = resources.pool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = static_cast<uint32_t>(resources.commands.size());
    if (vkAllocateCommandBuffers(resources.device, &commandInfo, resources.commands.data()) != VK_SUCCESS) return result;
    for (size_t index = 0; index < resources.commands.size(); ++index) {
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        if (vkBeginCommandBuffer(resources.commands[index], &beginInfo) != VK_SUCCESS) return result;
        VkClearValue clear{};
        clear.color.float32[0] = 0.05F;
        clear.color.float32[1] = 0.25F;
        clear.color.float32[2] = 0.45F;
        clear.color.float32[3] = 1.0F;
        VkRenderPassBeginInfo renderInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderInfo.renderPass = resources.renderPass;
        renderInfo.framebuffer = resources.framebuffers[index];
        renderInfo.renderArea.extent = extent;
        renderInfo.clearValueCount = 1;
        renderInfo.pClearValues = &clear;
        vkCmdBeginRenderPass(resources.commands[index], &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(resources.commands[index]);
        if (vkEndCommandBuffer(resources.commands[index]) != VK_SUCCESS) return result;
    }
    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vkCreateSemaphore(resources.device, &semaphoreInfo, nullptr, &resources.acquired) != VK_SUCCESS ||
        vkCreateSemaphore(resources.device, &semaphoreInfo, nullptr, &resources.rendered) != VK_SUCCESS ||
        vkCreateFence(resources.device, &fenceInfo, nullptr, &resources.fence) != VK_SUCCESS) return result;
    uint32_t imageIndex = 0;
    const VkResult acquireResult = vkAcquireNextImageKHR(resources.device, resources.swapchain, 5'000'000'000ULL, resources.acquired, VK_NULL_HANDLE, &imageIndex);
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) return result;
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &resources.acquired;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &resources.commands[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &resources.rendered;
    if (vkQueueSubmit(resources.queue, 1, &submitInfo, resources.fence) != VK_SUCCESS || vkWaitForFences(resources.device, 1, &resources.fence, VK_TRUE, 5'000'000'000ULL) != VK_SUCCESS) return result;
    result.frameSubmitted = true;
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &resources.rendered;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &resources.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    const VkResult presentResult = vkQueuePresentKHR(resources.queue, &presentInfo);
    if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR) return result;
    result.framePresented = true;
    return result;
}

} // namespace NeoEngine
