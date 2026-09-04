#include "Vulkan3DRenderer.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <vector>

namespace NeoEngine {
namespace {

#ifndef NEO_SHADER_DIR
#define NEO_SHADER_DIR "."
#endif

struct GpuVertex {
    float position[3];
    float normal[3];
    float uv[2];
};

struct Buffer {
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct BufferArena {
    Buffer buffer{};
    VkDeviceSize capacity = 0;
    VkDeviceSize used = 0;
    void* mapped = nullptr;
};

struct Frame {
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    BufferArena vertexArena{};
    BufferArena indexArena{};
};

uint32_t FindMemoryType(VkPhysicalDevice physical, uint32_t bits, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical, &properties);
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((bits & (1U << i)) &&
            (properties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }
    return UINT32_MAX;
}

bool CreateBuffer(VkPhysicalDevice physical, VkDevice device, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags,
                  Buffer& out) {
    out = {};
    if (size == 0) return false;

    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &info, nullptr, &out.handle) != VK_SUCCESS) {
        out = {};
        return false;
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(device, out.handle, &req);
    const uint32_t type = FindMemoryType(physical, req.memoryTypeBits, memoryFlags);
    if (type == UINT32_MAX) {
        vkDestroyBuffer(device, out.handle, nullptr);
        out = {};
        return false;
    }

    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = req.size;
    allocation.memoryTypeIndex = type;
    if (vkAllocateMemory(device, &allocation, nullptr, &out.memory) != VK_SUCCESS) {
        vkDestroyBuffer(device, out.handle, nullptr);
        out = {};
        return false;
    }

    if (vkBindBufferMemory(device, out.handle, out.memory, 0) != VK_SUCCESS) {
        vkFreeMemory(device, out.memory, nullptr);
        vkDestroyBuffer(device, out.handle, nullptr);
        out = {};
        return false;
    }
    return true;
}

void DestroyBuffer(VkDevice device, Buffer& buffer) {
    if (buffer.handle) vkDestroyBuffer(device, buffer.handle, nullptr);
    if (buffer.memory) vkFreeMemory(device, buffer.memory, nullptr);
    buffer = {};
}

void DestroyArena(VkDevice device, BufferArena& arena) {
    if (arena.mapped && arena.buffer.memory) {
        vkUnmapMemory(device, arena.buffer.memory);
    }
    arena.mapped = nullptr;
    DestroyBuffer(device, arena.buffer);
    arena.capacity = 0;
    arena.used = 0;
}

bool CreateArena(VkPhysicalDevice physical, VkDevice device, VkDeviceSize capacity,
                 VkBufferUsageFlags usage, BufferArena& arena) {
    DestroyArena(device, arena);
    if (!CreateBuffer(physical, device, capacity, usage,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, arena.buffer)) {
        return false;
    }
    if (vkMapMemory(device, arena.buffer.memory, 0, capacity, 0, &arena.mapped) != VK_SUCCESS) {
        DestroyBuffer(device, arena.buffer);
        return false;
    }
    arena.capacity = capacity;
    arena.used = 0;
    return true;
}

std::vector<uint32_t> ReadSpirv(const char* path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) return {};
    const std::streamsize size = stream.tellg();
    if (size <= 0 || size % 4 != 0) return {};
    std::vector<uint32_t> data(static_cast<size_t>(size) / 4U);
    stream.seekg(0);
    if (!stream.read(reinterpret_cast<char*>(data.data()), size)) return {};
    return data;
}

VkShaderModule CreateShader(VkDevice device, const std::vector<uint32_t>& code) {
    if (code.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size() * sizeof(uint32_t);
    info.pCode = code.data();
    VkShaderModule shader = VK_NULL_HANDLE;
    return vkCreateShaderModule(device, &info, nullptr, &shader) == VK_SUCCESS
        ? shader : VK_NULL_HANDLE;
}

bool HasExtension(VkPhysicalDevice physical, const char* name) {
    uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, nullptr) != VK_SUCCESS)
        return false;
    std::vector<VkExtensionProperties> extensions(count);
    if (count &&
        vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, extensions.data()) != VK_SUCCESS)
        return false;
    return std::any_of(extensions.begin(), extensions.end(),
                       [name](const auto& e) { return std::strcmp(e.extensionName, name) == 0; });
}

VkDeviceSize GrowCapacity(VkDeviceSize current, VkDeviceSize required) {
    VkDeviceSize capacity = current ? current : 256U * 1024U;
    while (capacity < required) {
        const VkDeviceSize next = capacity * 2U;
        if (next <= capacity) return required;
        capacity = next;
    }
    return capacity;
}

bool EnsureArena(VkPhysicalDevice physical, VkDevice device, BufferArena& arena,
                 VkDeviceSize required, VkBufferUsageFlags usage) {
    if (required <= arena.capacity) return true;
    return CreateArena(physical, device, GrowCapacity(arena.capacity, required), usage, arena);
}

} // namespace

struct Vulkan3DRenderer::Impl {
    SDL_Window* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsFamily = UINT32_MAX;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t presentFamily = UINT32_MAX;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    std::array<Frame, 2> frames{};
    uint32_t frameSlot = 0;
    uint32_t acquiredImageIndex = 0;
    bool frameBegun = false;

    void DestroySwapchainResources() {
        if (!device) return;
        vkDeviceWaitIdle(device);
        for (auto f : framebuffers) vkDestroyFramebuffer(device, f, nullptr);
        framebuffers.clear();
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
        if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
        if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
        if (depthView) vkDestroyImageView(device, depthView, nullptr);
        depthView = VK_NULL_HANDLE;
        if (depthImage) vkDestroyImage(device, depthImage, nullptr);
        depthImage = VK_NULL_HANDLE;
        if (depthMemory) vkFreeMemory(device, depthMemory, nullptr);
        depthMemory = VK_NULL_HANDLE;
        for (auto v : swapchainViews) vkDestroyImageView(device, v, nullptr);
        swapchainViews.clear();
        swapchainImages.clear();
        if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }

    void Destroy() {
        if (device) vkDeviceWaitIdle(device);
        for (auto& frame : frames) {
            DestroyArena(device, frame.vertexArena);
            DestroyArena(device, frame.indexArena);
        }
        DestroySwapchainResources();
        for (auto& f : frames) {
            if (f.fence) vkDestroyFence(device, f.fence, nullptr);
            if (f.imageAvailable) vkDestroySemaphore(device, f.imageAvailable, nullptr);
            if (f.renderFinished) vkDestroySemaphore(device, f.renderFinished, nullptr);
        }
        if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
        if (device) vkDestroyDevice(device, nullptr);
        if (surface && instance) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (instance) vkDestroyInstance(instance, nullptr);
        if (window) SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_Quit();
        *this = {};
    }
};

Vulkan3DRenderer::~Vulkan3DRenderer() { Reset(); }

bool Vulkan3DRenderer::Initialize(uint32_t width, uint32_t height, const char* title) {
    Reset();
    if (width == 0 || height == 0 || width > 16384 || height > 16384 || !title) {
        lastError_ = Vulkan3DRendererError::InvalidConfiguration;
        return false;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        lastError_ = Vulkan3DRendererError::SdlFailure;
        return false;
    }

    auto impl = std::make_unique<Impl>();
    impl->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    static_cast<int>(width), static_cast<int>(height),
                                    SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!impl->window) {
        lastError_ = Vulkan3DRendererError::SdlFailure;
        SDL_Quit();
        return false;
    }

    unsigned extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(impl->window, &extensionCount, nullptr) != SDL_TRUE ||
        extensionCount == 0) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }
    std::vector<const char*> extensions(extensionCount);
    if (SDL_Vulkan_GetInstanceExtensions(impl->window, &extensionCount, extensions.data()) != SDL_TRUE) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = title;
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "FauzanEngine2";
    app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &app;
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    if (vkCreateInstance(&instanceInfo, nullptr, &impl->instance) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }
    if (SDL_Vulkan_CreateSurface(impl->window, impl->instance, &impl->surface) != SDL_TRUE) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }

    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(impl->instance, &deviceCount, nullptr) != VK_SUCCESS ||
        deviceCount == 0) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (vkEnumeratePhysicalDevices(impl->instance, &deviceCount, devices.data()) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }

    for (VkPhysicalDevice device : devices) {
        if (!HasExtension(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());
        uint32_t graphics = UINT32_MAX;
        uint32_t present = UINT32_MAX;
        for (uint32_t i = 0; i < familyCount; ++i) {
            VkBool32 supportsPresent = VK_FALSE;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i, impl->surface, &supportsPresent) != VK_SUCCESS)
                continue;
            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphics == UINT32_MAX)
                graphics = i;
            if (supportsPresent && present == UINT32_MAX) present = i;
        }
        if (graphics != UINT32_MAX && present != UINT32_MAX) {
            impl->physical = device;
            impl->graphicsFamily = graphics;
            impl->presentFamily = present;
            break;
        }
    }

    if (!impl->physical) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }

    std::vector<VkDeviceQueueCreateInfo> queues;
    const float priority = 1.0F;
    VkDeviceQueueCreateInfo graphicsQueue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    graphicsQueue.queueFamilyIndex = impl->graphicsFamily;
    graphicsQueue.queueCount = 1;
    graphicsQueue.pQueuePriorities = &priority;
    queues.push_back(graphicsQueue);
    if (impl->presentFamily != impl->graphicsFamily) {
        VkDeviceQueueCreateInfo presentQueue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        presentQueue.queueFamilyIndex = impl->presentFamily;
        presentQueue.queueCount = 1;
        presentQueue.pQueuePriorities = &priority;
        queues.push_back(presentQueue);
    }

    const char* deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkPhysicalDeviceFeatures features{};
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    deviceInfo.pQueueCreateInfos = queues.data();
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = &deviceExtension;
    deviceInfo.pEnabledFeatures = &features;
    if (vkCreateDevice(impl->physical, &deviceInfo, nullptr, &impl->device) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }
    vkGetDeviceQueue(impl->device, impl->graphicsFamily, 0, &impl->graphicsQueue);
    vkGetDeviceQueue(impl->device, impl->presentFamily, 0, &impl->presentQueue);

    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.queueFamilyIndex = impl->graphicsFamily;
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(impl->device, &pool, nullptr, &impl->commandPool) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        impl->Destroy();
        return false;
    }

    for (auto& frame : impl->frames) {
        VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (vkCreateSemaphore(impl->device, &semaphore, nullptr, &frame.imageAvailable) != VK_SUCCESS ||
            vkCreateSemaphore(impl->device, &semaphore, nullptr, &frame.renderFinished) != VK_SUCCESS ||
            vkCreateFence(impl->device, &fence, nullptr, &frame.fence) != VK_SUCCESS) {
            lastError_ = Vulkan3DRendererError::VulkanFailure;
            impl->Destroy();
            return false;
        }
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocation.commandPool = impl->commandPool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(impl->device, &allocation, &frame.commandBuffer) != VK_SUCCESS) {
            lastError_ = Vulkan3DRendererError::VulkanFailure;
            impl->Destroy();
            return false;
        }
    }

    impl_ = impl.release();
    if (!Resize(width, height)) {
        Reset();
        return false;
    }
    ready_ = true;
    lastError_ = Vulkan3DRendererError::None;
    return true;
}

bool Vulkan3DRenderer::Resize(uint32_t width, uint32_t height) {
    if (!impl_ || width == 0 || height == 0) {
        lastError_ = Vulkan3DRendererError::InvalidConfiguration;
        return false;
    }

    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(impl_->physical, impl_->surface, &capabilities) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }

    uint32_t formatCount = 0;
    uint32_t modeCount = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(impl_->physical, impl_->surface, &formatCount, nullptr) != VK_SUCCESS ||
        vkGetPhysicalDeviceSurfacePresentModesKHR(impl_->physical, impl_->surface, &modeCount, nullptr) != VK_SUCCESS ||
        formatCount == 0 || modeCount == 0) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(impl_->physical, impl_->surface, &formatCount, formats.data()) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }
    auto format = formats.front();
    for (const auto& candidate : formats) {
        if (candidate.format == VK_FORMAT_B8G8R8A8_SRGB &&
            candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = candidate;
            break;
        }
    }

    VkExtent2D extent{};
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = capabilities.currentExtent;
    } else {
        extent = {
            std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    impl_->DestroySwapchainResources();

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swap{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swap.surface = impl_->surface;
    swap.minImageCount = imageCount;
    swap.imageFormat = format.format;
    swap.imageColorSpace = format.colorSpace;
    swap.imageExtent = extent;
    swap.imageArrayLayers = 1;
    swap.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t families[] = {impl_->graphicsFamily, impl_->presentFamily};
    if (impl_->graphicsFamily != impl_->presentFamily) {
        swap.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swap.queueFamilyIndexCount = 2;
        swap.pQueueFamilyIndices = families;
    } else {
        swap.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swap.preTransform = capabilities.currentTransform;
    swap.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swap.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(impl_->device, &swap, nullptr, &impl_->swapchain) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }
    impl_->swapchainFormat = format.format;
    impl_->extent = extent;

    uint32_t actualCount = 0;
    if (vkGetSwapchainImagesKHR(impl_->device, impl_->swapchain, &actualCount, nullptr) != VK_SUCCESS ||
        actualCount == 0) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }
    impl_->swapchainImages.resize(actualCount);
    if (vkGetSwapchainImagesKHR(impl_->device, impl_->swapchain, &actualCount, impl_->swapchainImages.data()) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }
    impl_->swapchainViews.resize(actualCount);
    for (uint32_t i = 0; i < actualCount; ++i) {
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = impl_->swapchainImages[i];
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = impl_->swapchainFormat;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = 1;
        if (vkCreateImageView(impl_->device, &view, nullptr, &impl_->swapchainViews[i]) != VK_SUCCESS) {
            lastError_ = Vulkan3DRendererError::VulkanFailure;
            return false;
        }
    }

    VkImageCreateInfo depth{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    depth.imageType = VK_IMAGE_TYPE_2D;
    depth.format = impl_->depthFormat;
    depth.extent = {extent.width, extent.height, 1};
    depth.mipLevels = 1;
    depth.arrayLayers = 1;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(impl_->device, &depth, nullptr, &impl_->depthImage) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }

    VkMemoryRequirements depthReq{};
    vkGetImageMemoryRequirements(impl_->device, impl_->depthImage, &depthReq);
    const uint32_t depthType = FindMemoryType(
        impl_->physical, depthReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (depthType == UINT32_MAX) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }
    VkMemoryAllocateInfo depthAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    depthAlloc.allocationSize = depthReq.size;
    depthAlloc.memoryTypeIndex = depthType;
    if (vkAllocateMemory(impl_->device, &depthAlloc, nullptr, &impl_->depthMemory) != VK_SUCCESS ||
        vkBindImageMemory(impl_->device, impl_->depthImage, impl_->depthMemory, 0) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }

    VkImageViewCreateInfo depthView{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    depthView.image = impl_->depthImage;
    depthView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthView.format = impl_->depthFormat;
    depthView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthView.subresourceRange.levelCount = 1;
    depthView.subresourceRange.layerCount = 1;
    if (vkCreateImageView(impl_->device, &depthView, nullptr, &impl_->depthView) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }

    VkAttachmentDescription attachments[2]{};
    attachments[0].format = impl_->swapchainFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[1].format = impl_->depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass.attachmentCount = 2;
    pass.pAttachments = attachments;
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    pass.dependencyCount = 1;
    pass.pDependencies = &dependency;
    if (vkCreateRenderPass(impl_->device, &pass, nullptr, &impl_->renderPass) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::VulkanFailure;
        return false;
    }

    auto vertCode = ReadSpirv(NEO_SHADER_DIR "/neo_mesh.vert.spv");
    auto fragCode = ReadSpirv(NEO_SHADER_DIR "/neo_mesh.frag.spv");
    VkShaderModule vert = CreateShader(impl_->device, vertCode);
    VkShaderModule frag = CreateShader(impl_->device, fragCode);
    if (!vert || !frag) {
        if (vert) vkDestroyShaderModule(impl_->device, vert, nullptr);
        if (frag) vkDestroyShaderModule(impl_->device, frag, nullptr);
        lastError_ = Vulkan3DRendererError::ShaderUnavailable;
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(GpuVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription attributes[3]{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, 24}
    };

    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    input.vertexBindingDescriptionCount = 1;
    input.pVertexBindingDescriptions = &binding;
    input.vertexAttributeDescriptionCount = 3;
    input.pVertexAttributeDescriptions = attributes;

    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0F;

    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthState.depthTestEnable = VK_TRUE;
    depthState.depthWriteEnable = VK_TRUE;
    depthState.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = 0xF;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;

    VkDynamicState dynamicStates[2]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamicStates;

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push.offset = 0;
    push.size = sizeof(float) * 16U;
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    if (vkCreatePipelineLayout(impl_->device, &layout, nullptr, &impl_->pipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(impl_->device, vert, nullptr);
        vkDestroyShaderModule(impl_->device, frag, nullptr);
        lastError_ = Vulkan3DRendererError::PipelineFailure;
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline.stageCount = 2;
    pipeline.pStages = stages;
    pipeline.pVertexInputState = &input;
    pipeline.pInputAssemblyState = &assembly;
    pipeline.pViewportState = &viewport;
    pipeline.pRasterizationState = &raster;
    pipeline.pMultisampleState = &multisample;
    pipeline.pDepthStencilState = &depthState;
    pipeline.pColorBlendState = &blend;
    pipeline.pDynamicState = &dynamic;
    pipeline.layout = impl_->pipelineLayout;
    pipeline.renderPass = impl_->renderPass;
    pipeline.subpass = 0;
    if (vkCreateGraphicsPipelines(impl_->device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &impl_->pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(impl_->device, vert, nullptr);
        vkDestroyShaderModule(impl_->device, frag, nullptr);
        lastError_ = Vulkan3DRendererError::PipelineFailure;
        return false;
    }
    vkDestroyShaderModule(impl_->device, vert, nullptr);
    vkDestroyShaderModule(impl_->device, frag, nullptr);

    impl_->framebuffers.resize(impl_->swapchainViews.size());
    for (size_t i = 0; i < impl_->swapchainViews.size(); ++i) {
        VkImageView attachmentsForFrame[2]{impl_->swapchainViews[i], impl_->depthView};
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass = impl_->renderPass;
        framebuffer.attachmentCount = 2;
        framebuffer.pAttachments = attachmentsForFrame;
        framebuffer.width = extent.width;
        framebuffer.height = extent.height;
        framebuffer.layers = 1;
        if (vkCreateFramebuffer(impl_->device, &framebuffer, nullptr, &impl_->framebuffers[i]) != VK_SUCCESS) {
            lastError_ = Vulkan3DRendererError::VulkanFailure;
            return false;
        }
    }

    stats_.width = extent.width;
    stats_.height = extent.height;
    return true;
}

bool Vulkan3DRenderer::BeginFrame(float clearR, float clearG, float clearB, float clearA) {
    if (!ready_ || !impl_ || impl_->frameBegun) {
        lastError_ = Vulkan3DRendererError::FrameFailure;
        return false;
    }
    Frame& frame = impl_->frames[impl_->frameSlot];
    const VkResult wait = vkWaitForFences(impl_->device, 1, &frame.fence, VK_TRUE, UINT64_MAX);
    if (wait != VK_SUCCESS) {
        lastError_ = wait == VK_ERROR_DEVICE_LOST ? Vulkan3DRendererError::DeviceLost
                                                   : Vulkan3DRendererError::FrameFailure;
        return false;
    }

    const VkResult acquire = vkAcquireNextImageKHR(impl_->device, impl_->swapchain, UINT64_MAX,
                                                    frame.imageAvailable, VK_NULL_HANDLE,
                                                    &impl_->acquiredImageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        lastError_ = Vulkan3DRendererError::SwapchainOutOfDate;
        return false;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        lastError_ = acquire == VK_ERROR_DEVICE_LOST ? Vulkan3DRendererError::DeviceLost
                                                      : Vulkan3DRendererError::FrameFailure;
        return false;
    }

    vkResetFences(impl_->device, 1, &frame.fence);
    vkResetCommandBuffer(frame.commandBuffer, 0);
    frame.vertexArena.used = 0;
    frame.indexArena.used = 0;

    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(frame.commandBuffer, &begin) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::FrameFailure;
        return false;
    }

    VkClearValue clear[2]{};
    clear[0].color = {{clearR, clearG, clearB, clearA}};
    clear[1].depthStencil = {1.0F, 0};
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = impl_->renderPass;
    pass.framebuffer = impl_->framebuffers[impl_->acquiredImageIndex];
    pass.renderArea.extent = impl_->extent;
    pass.clearValueCount = 2;
    pass.pClearValues = clear;
    vkCmdBeginRenderPass(frame.commandBuffer, &pass, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{
        0, 0, static_cast<float>(impl_->extent.width),
        static_cast<float>(impl_->extent.height), 0, 1
    };
    VkRect2D scissor{{0, 0}, impl_->extent};
    vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, impl_->pipeline);

    stats_.vertexCount = 0;
    stats_.indexCount = 0;
    impl_->frameBegun = true;
    return true;
}

bool Vulkan3DRenderer::DrawIndexed(std::span<const Vulkan3DVertex> vertices,
                                   std::span<const uint32_t> indices,
                                   const float* mvp) {
    if (!impl_ || !impl_->frameBegun || vertices.empty() || indices.empty() || !mvp ||
        indices.size() % 3U != 0U) {
        lastError_ = Vulkan3DRendererError::FrameFailure;
        return false;
    }
    if (vertices.size() > 1000000U || indices.size() > 3000000U) {
        lastError_ = Vulkan3DRendererError::BufferFailure;
        return false;
    }

    const VkDeviceSize vertexBytes = vertices.size() * sizeof(GpuVertex);
    const VkDeviceSize indexBytes = indices.size() * sizeof(uint32_t);
    Frame& frame = impl_->frames[impl_->frameSlot];

    // Each frame owns a persistent mapped arena. Because the frame fence has completed
    // before BeginFrame resets it, the arena can be reused without per-draw allocation.
    const VkDeviceSize vertexRequired = frame.vertexArena.used + vertexBytes;
    const VkDeviceSize indexRequired = frame.indexArena.used + indexBytes;

    // Capacity growth is only legal before the first draw in a frame. Growing after
    // commands have already referenced the old VkBuffer would invalidate those commands.
    if (vertexRequired > frame.vertexArena.capacity) {
        if (frame.vertexArena.used != 0 ||
            !EnsureArena(impl_->physical, impl_->device, frame.vertexArena,
                         vertexRequired, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) {
            lastError_ = Vulkan3DRendererError::BufferFailure;
            return false;
        }
    }
    if (indexRequired > frame.indexArena.capacity) {
        if (frame.indexArena.used != 0 ||
            !EnsureArena(impl_->physical, impl_->device, frame.indexArena,
                         indexRequired, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)) {
            lastError_ = Vulkan3DRendererError::BufferFailure;
            return false;
        }
    }

    const VkDeviceSize vertexOffset = frame.vertexArena.used;
    const VkDeviceSize indexOffset = frame.indexArena.used;
    auto* dstVertices = static_cast<GpuVertex*>(
        static_cast<std::byte*>(frame.vertexArena.mapped) + vertexOffset);
    for (size_t i = 0; i < vertices.size(); ++i) {
        dstVertices[i] = {
            {vertices[i].px, vertices[i].py, vertices[i].pz},
            {vertices[i].nx, vertices[i].ny, vertices[i].nz},
            {vertices[i].u, vertices[i].v}
        };
    }
    std::memcpy(static_cast<std::byte*>(frame.indexArena.mapped) + indexOffset,
                indices.data(), static_cast<size_t>(indexBytes));

    // Indices remain local to this draw. The vertex-buffer binding offset selects the
    // corresponding vertex range, so no CPU-side index rewrite is needed.
    vkCmdPushConstants(frame.commandBuffer, impl_->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(float) * 16U, mvp);
    VkDeviceSize boundVertexOffset = vertexOffset;
    vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, &frame.vertexArena.buffer.handle,
                           &boundVertexOffset);
    vkCmdBindIndexBuffer(frame.commandBuffer, frame.indexArena.buffer.handle,
                         indexOffset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(frame.commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    frame.vertexArena.used = vertexRequired;
    frame.indexArena.used = indexRequired;
    stats_.vertexCount += static_cast<uint32_t>(vertices.size());
    stats_.indexCount += static_cast<uint32_t>(indices.size());
    return true;
}

bool Vulkan3DRenderer::EndFrame() {
    if (!impl_ || !impl_->frameBegun) {
        lastError_ = Vulkan3DRendererError::FrameFailure;
        return false;
    }

    Frame& frame = impl_->frames[impl_->frameSlot];
    vkCmdEndRenderPass(frame.commandBuffer);
    if (vkEndCommandBuffer(frame.commandBuffer) != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::FrameFailure;
        impl_->frameBegun = false;
        return false;
    }

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame.imageAvailable;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &frame.commandBuffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame.renderFinished;

    const VkResult submitted = vkQueueSubmit(impl_->graphicsQueue, 1, &submit, frame.fence);
    if (submitted != VK_SUCCESS) {
        lastError_ = submitted == VK_ERROR_DEVICE_LOST ? Vulkan3DRendererError::DeviceLost
                                                        : Vulkan3DRendererError::FrameFailure;
        impl_->frameBegun = false;
        return false;
    }

    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &frame.renderFinished;
    present.swapchainCount = 1;
    present.pSwapchains = &impl_->swapchain;
    present.pImageIndices = &impl_->acquiredImageIndex;
    const VkResult presented = vkQueuePresentKHR(impl_->presentQueue, &present);

    impl_->frameBegun = false;
    impl_->frameSlot = (impl_->frameSlot + 1U) % static_cast<uint32_t>(impl_->frames.size());
    stats_.frameIndex++;

    if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR) {
        lastError_ = Vulkan3DRendererError::SwapchainOutOfDate;
        return false;
    }
    if (presented == VK_ERROR_DEVICE_LOST) {
        lastError_ = Vulkan3DRendererError::DeviceLost;
        return false;
    }
    if (presented != VK_SUCCESS) {
        lastError_ = Vulkan3DRendererError::FrameFailure;
        return false;
    }

    lastError_ = Vulkan3DRendererError::None;
    return true;
}

void Vulkan3DRenderer::Reset() {
    ready_ = false;
    if (impl_) {
        impl_->Destroy();
        delete impl_;
        impl_ = nullptr;
    }
    stats_ = {};
    lastError_ = Vulkan3DRendererError::None;
}

} // namespace NeoEngine
