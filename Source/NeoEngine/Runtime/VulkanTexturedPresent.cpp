#include "VulkanTexturedPresent.h"

#include "TextureStaging.h"
#include "SoftwareRenderer.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace NeoEngine {
namespace {

constexpr VkFormat kTextureFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr uint64_t kMaxBytes = 64ULL * 1024ULL * 1024ULL;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;

uint64_t Hash(const std::vector<uint8_t>& bytes) {
    uint64_t value = kHashOffset;
    for (const uint8_t byte : bytes) {
        value ^= byte;
        value *= kHashPrime;
    }
    return value;
}

uint32_t FindMemoryType(VkPhysicalDevice physical, uint32_t typeBits, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical, &properties);
    for (uint32_t index = 0; index < properties.memoryTypeCount; ++index) {
        if ((typeBits & (1U << index)) != 0 && (properties.memoryTypes[index].propertyFlags & flags) == flags) return index;
    }
    return UINT32_MAX;
}

bool CreateBuffer(VkPhysicalDevice physical, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &info, nullptr, &buffer) != VK_SUCCESS) return false;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    const uint32_t type = FindMemoryType(physical, requirements.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (type == UINT32_MAX) return false;
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = type;
    if (vkAllocateMemory(device, &allocation, nullptr, &memory) != VK_SUCCESS) return false;
    return vkBindBufferMemory(device, buffer, memory, 0) == VK_SUCCESS;
}

bool CreateTextureImage(VkPhysicalDevice physical, VkDevice device, uint32_t width, uint32_t height,
                        VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = kTextureFormat;
    info.extent = {width, height, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device, &info, nullptr, &image) != VK_SUCCESS) return false;
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device, image, &requirements);
    const uint32_t type = FindMemoryType(physical, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (type == UINT32_MAX) return false;
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = type;
    if (vkAllocateMemory(device, &allocation, nullptr, &memory) != VK_SUCCESS) return false;
    return vkBindImageMemory(device, image, memory, 0) == VK_SUCCESS;
}

VkImageViewCreateInfo ViewInfo(VkImage image, VkFormat format) {
    VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.layerCount = 1;
    return info;
}

struct Resources {
    SDL_Window* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamily = UINT32_MAX;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkBuffer uploadBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uploadMemory = VK_NULL_HANDLE;
    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureMemory = VK_NULL_HANDLE;
    VkImageView textureView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    ~Resources() {
        if (device != VK_NULL_HANDLE) vkDeviceWaitIdle(device);
        if (fence != VK_NULL_HANDLE) vkDestroyFence(device, fence, nullptr);
        if (renderFinished != VK_NULL_HANDLE) vkDestroySemaphore(device, renderFinished, nullptr);
        if (imageAvailable != VK_NULL_HANDLE) vkDestroySemaphore(device, imageAvailable, nullptr);
        if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, pipeline, nullptr);
        if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        if (descriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        if (sampler != VK_NULL_HANDLE) vkDestroySampler(device, sampler, nullptr);
        if (textureView != VK_NULL_HANDLE) vkDestroyImageView(device, textureView, nullptr);
        if (textureImage != VK_NULL_HANDLE) vkDestroyImage(device, textureImage, nullptr);
        if (textureMemory != VK_NULL_HANDLE) vkFreeMemory(device, textureMemory, nullptr);
        if (uploadBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, uploadBuffer, nullptr);
        if (uploadMemory != VK_NULL_HANDLE) vkFreeMemory(device, uploadMemory, nullptr);
        for (VkFramebuffer framebuffer : framebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(device, renderPass, nullptr);
        for (VkImageView view : swapchainViews) vkDestroyImageView(device, view, nullptr);
        if (swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, swapchain, nullptr);
        if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, commandPool, nullptr);
        if (device != VK_NULL_HANDLE) vkDestroyDevice(device, nullptr);
        if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, nullptr);
        if (window != nullptr) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

bool SelectDevice(Resources& resources) {
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
            VkBool32 present = VK_FALSE;
            if ((families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || families[family].queueCount == 0 ||
                vkGetPhysicalDeviceSurfaceSupportKHR(candidate, family, resources.surface, &present) != VK_SUCCESS || present != VK_TRUE) continue;
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
            if (vkCreateDevice(candidate, &deviceInfo, nullptr, &resources.device) != VK_SUCCESS) continue;
            resources.physical = candidate;
            resources.queueFamily = family;
            vkGetDeviceQueue(resources.device, family, 0, &resources.queue);
            return resources.queue != VK_NULL_HANDLE;
        }
    }
    return false;
}

std::vector<uint32_t> ReadSpirv(const char* path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) return {};
    const std::streamsize size = stream.tellg();
    if (size <= 0 || size % static_cast<std::streamsize>(sizeof(uint32_t)) != 0) return {};
    std::vector<uint32_t> words(static_cast<size_t>(size) / sizeof(uint32_t));
    stream.seekg(0);
    if (!stream.read(reinterpret_cast<char*>(words.data()), size)) return {};
    return words;
}

VkSurfaceFormatKHR ChooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    const auto preferred = std::find_if(formats.begin(), formats.end(), [](const VkSurfaceFormatKHR& format) {
        return format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    return preferred == formats.end() ? formats.front() : *preferred;
}

VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;
    return {std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

} // namespace

VulkanPresentStatus VulkanTexturedPresentProbe::ClassifyDriverResult(int32_t result) {
    if (result == static_cast<int32_t>(VK_SUCCESS) || result == static_cast<int32_t>(VK_SUBOPTIMAL_KHR)) return VulkanPresentStatus::None;
    if (result == static_cast<int32_t>(VK_ERROR_DEVICE_LOST)) return VulkanPresentStatus::DeviceLost;
    if (result == static_cast<int32_t>(VK_ERROR_OUT_OF_DATE_KHR)) return VulkanPresentStatus::SurfaceOutOfDate;
    if (result == static_cast<int32_t>(VK_TIMEOUT)) return VulkanPresentStatus::Timeout;
    return VulkanPresentStatus::DriverRejected;
}

VulkanTexturedPresentResult VulkanTexturedPresentProbe::Present(std::span<const uint32_t> pixels, uint32_t width, uint32_t height) {
    VulkanTexturedPresentResult result{};
    const uint64_t expectedPixels = static_cast<uint64_t>(width) * height;
    if (width == 0U || height == 0U || width > 2048U || height > 2048U || expectedPixels == 0U || pixels.size() != expectedPixels) {
        result.status = VulkanPresentStatus::InvalidInput;
        return result;
    }
    RgbaTexture texture{};
    texture.width = static_cast<uint16_t>(width);
    texture.height = static_cast<uint16_t>(height);
    texture.rgba.resize(static_cast<size_t>(expectedPixels) * 4U);
    for (size_t index = 0U; index < pixels.size(); ++index) {
        const uint32_t pixel = pixels[index];
        texture.rgba[index * 4U + 0U] = static_cast<uint8_t>((pixel >> 24U) & 0xFFU);
        texture.rgba[index * 4U + 1U] = static_cast<uint8_t>((pixel >> 16U) & 0xFFU);
        texture.rgba[index * 4U + 2U] = static_cast<uint8_t>((pixel >> 8U) & 0xFFU);
        texture.rgba[index * 4U + 3U] = static_cast<uint8_t>(pixel & 0xFFU);
    }
    return Present(texture, width, height);
}

VulkanTexturedPresentResult VulkanTexturedPresentProbe::Present(const CpuTextureResource& texture, uint32_t width, uint32_t height) {
    VulkanTexturedPresentResult result{};
    const uint64_t expectedBytes = static_cast<uint64_t>(texture.width) * texture.height * 4U;
    if (texture.assetId.empty() || texture.sourceHash == 0 || texture.width == 0 || texture.height == 0 ||
        expectedBytes == 0 || texture.rgba.size() != expectedBytes) { result.status = VulkanPresentStatus::InvalidInput; return result; }
    result = Present(RgbaTexture{texture.width, texture.height, texture.rgba}, width, height);
    if (result.framePresented) result.stagedSourceHash = texture.sourceHash;
    return result;
}

VulkanTexturedPresentResult VulkanTexturedPresentProbe::Present(const RgbaTexture& texture, uint32_t width, uint32_t height) {
    VulkanTexturedPresentResult result{};
    const uint64_t bytes = static_cast<uint64_t>(texture.width) * texture.height * 4U;
    if (width == 0 || height == 0 || width > 2048 || height > 2048 || texture.width == 0 || texture.height == 0 ||
        bytes == 0 || bytes > kMaxBytes || texture.rgba.size() != bytes) { result.status = VulkanPresentStatus::InvalidInput; return result; }
    result.textureHash = Hash(texture.rgba);
    result.status = VulkanPresentStatus::Unavailable;
    Resources resources{};
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return result;
    resources.window = SDL_CreateWindow("NeoEngine Textured Present Probe", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    if (resources.window == nullptr) return result;
    result.windowCreated = true;
    unsigned extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(resources.window, &extensionCount, nullptr) != SDL_TRUE || extensionCount == 0) return result;
    std::vector<const char*> extensions(extensionCount);
    if (SDL_Vulkan_GetInstanceExtensions(resources.window, &extensionCount, extensions.data()) != SDL_TRUE) return result;
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "NeoEngineTexturedPresentProbe";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &app;
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    if (vkCreateInstance(&instanceInfo, nullptr, &resources.instance) != VK_SUCCESS ||
        SDL_Vulkan_CreateSurface(resources.window, resources.instance, &resources.surface) != SDL_TRUE) return result;
    result.surfaceCreated = true;
    if (!SelectDevice(resources)) return result;
    result.deviceCreated = true;
    VkSurfaceCapabilitiesKHR capabilities{};
    uint32_t formatCount = 0;
    uint32_t presentModeCount = 0;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(resources.physical, resources.surface, &capabilities) != VK_SUCCESS ||
        vkGetPhysicalDeviceSurfaceFormatsKHR(resources.physical, resources.surface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0 ||
        vkGetPhysicalDeviceSurfacePresentModesKHR(resources.physical, resources.surface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) return result;
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(resources.physical, resources.surface, &formatCount, formats.data()) != VK_SUCCESS) return result;
    const VkSurfaceFormatKHR format = ChooseFormat(formats);
    resources.swapchainFormat = format.format;
    resources.extent = ChooseExtent(capabilities, width, height);
    uint32_t imageCount = capabilities.minImageCount + 1U;
    if (capabilities.maxImageCount != 0 && imageCount > capabilities.maxImageCount) imageCount = capabilities.maxImageCount;
    VkSwapchainCreateInfoKHR swapchainInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = resources.surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = format.format;
    swapchainInfo.imageColorSpace = format.colorSpace;
    swapchainInfo.imageExtent = resources.extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(resources.device, &swapchainInfo, nullptr, &resources.swapchain) != VK_SUCCESS) return result;
    uint32_t swapchainCount = 0;
    if (vkGetSwapchainImagesKHR(resources.device, resources.swapchain, &swapchainCount, nullptr) != VK_SUCCESS || swapchainCount == 0) return result;
    resources.swapchainImages.resize(swapchainCount);
    if (vkGetSwapchainImagesKHR(resources.device, resources.swapchain, &swapchainCount, resources.swapchainImages.data()) != VK_SUCCESS) return result;
    result.swapchainCreated = true;
    result.imageCount = swapchainCount;
    for (VkImage image : resources.swapchainImages) {
        VkImageView view = VK_NULL_HANDLE;
        const VkImageViewCreateInfo viewInfo = ViewInfo(image, resources.swapchainFormat);
        if (vkCreateImageView(resources.device, &viewInfo, nullptr, &view) != VK_SUCCESS) return result;
        resources.swapchainViews.push_back(view);
    }
    VkAttachmentDescription attachment{};
    attachment.format = resources.swapchainFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    if (vkCreateRenderPass(resources.device, &renderPassInfo, nullptr, &resources.renderPass) != VK_SUCCESS) return result;
    for (VkImageView view : resources.swapchainViews) {
        VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = resources.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &view;
        framebufferInfo.width = resources.extent.width;
        framebufferInfo.height = resources.extent.height;
        framebufferInfo.layers = 1;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (vkCreateFramebuffer(resources.device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) return result;
        resources.framebuffers.push_back(framebuffer);
    }
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = resources.queueFamily;
    if (vkCreateCommandPool(resources.device, &poolInfo, nullptr, &resources.commandPool) != VK_SUCCESS) return result;
    resources.commandBuffers.resize(resources.swapchainImages.size());
    VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = resources.commandPool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = static_cast<uint32_t>(resources.commandBuffers.size());
    if (vkAllocateCommandBuffers(resources.device, &commandInfo, resources.commandBuffers.data()) != VK_SUCCESS) return result;
    if (!CreateBuffer(resources.physical, resources.device, static_cast<VkDeviceSize>(bytes), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      resources.uploadBuffer, resources.uploadMemory) ||
        !CreateTextureImage(resources.physical, resources.device, texture.width, texture.height, resources.textureImage, resources.textureMemory)) return result;
    void* mapped = nullptr;
    if (vkMapMemory(resources.device, resources.uploadMemory, 0, static_cast<VkDeviceSize>(bytes), 0, &mapped) != VK_SUCCESS || mapped == nullptr) return result;
    std::memcpy(mapped, texture.rgba.data(), texture.rgba.size());
    vkUnmapMemory(resources.device, resources.uploadMemory);
    const VkImageViewCreateInfo textureViewInfo = ViewInfo(resources.textureImage, kTextureFormat);
    if (vkCreateImageView(resources.device, &textureViewInfo, nullptr, &resources.textureView) != VK_SUCCESS) return result;
    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0F;
    if (vkCreateSampler(resources.device, &samplerInfo, nullptr, &resources.sampler) != VK_SUCCESS) return result;
    result.textureUploaded = true;
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    if (vkCreateDescriptorSetLayout(resources.device, &layoutInfo, nullptr, &resources.descriptorSetLayout) != VK_SUCCESS) return result;
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
    VkDescriptorPoolCreateInfo descriptorPoolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolInfo.maxSets = 1;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(resources.device, &descriptorPoolInfo, nullptr, &resources.descriptorPool) != VK_SUCCESS) return result;
    VkDescriptorSetAllocateInfo allocationInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocationInfo.descriptorPool = resources.descriptorPool;
    allocationInfo.descriptorSetCount = 1;
    allocationInfo.pSetLayouts = &resources.descriptorSetLayout;
    if (vkAllocateDescriptorSets(resources.device, &allocationInfo, &resources.descriptorSet) != VK_SUCCESS) return result;
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = resources.sampler;
    imageInfo.imageView = resources.textureView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = resources.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(resources.device, 1, &descriptorWrite, 0, nullptr);
    const std::vector<uint32_t> vertexCode = ReadSpirv(NEO_SHADER_DIR "/neo_texture.vert.spv");
    const std::vector<uint32_t> fragmentCode = ReadSpirv(NEO_SHADER_DIR "/neo_texture.frag.spv");
    if (vertexCode.empty() || fragmentCode.empty()) return result;
    VkShaderModuleCreateInfo shaderInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderInfo.codeSize = vertexCode.size() * sizeof(uint32_t);
    shaderInfo.pCode = vertexCode.data();
    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;
    if (vkCreateShaderModule(resources.device, &shaderInfo, nullptr, &vertexShader) != VK_SUCCESS) return result;
    shaderInfo.codeSize = fragmentCode.size() * sizeof(uint32_t);
    shaderInfo.pCode = fragmentCode.data();
    if (vkCreateShaderModule(resources.device, &shaderInfo, nullptr, &fragmentShader) != VK_SUCCESS) {
        vkDestroyShaderModule(resources.device, vertexShader, nullptr);
        return result;
    }
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertexShader;
    stages[0].pName = "main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragmentShader;
    stages[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{};
    viewport.width = static_cast<float>(resources.extent.width);
    viewport.height = static_cast<float>(resources.extent.height);
    viewport.maxDepth = 1.0F;
    VkRect2D scissor{};
    scissor.extent = resources.extent;
    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo rasterization{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0F;
    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &resources.descriptorSetLayout;
    if (vkCreatePipelineLayout(resources.device, &pipelineLayoutInfo, nullptr, &resources.pipelineLayout) != VK_SUCCESS) return result;
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterization;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &blend;
    pipelineInfo.layout = resources.pipelineLayout;
    pipelineInfo.renderPass = resources.renderPass;
    const VkResult pipelineResult = vkCreateGraphicsPipelines(resources.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resources.pipeline);
    vkDestroyShaderModule(resources.device, fragmentShader, nullptr);
    vkDestroyShaderModule(resources.device, vertexShader, nullptr);
    if (pipelineResult != VK_SUCCESS) return result;
    result.pipelineCreated = true;
    for (size_t index = 0; index < resources.commandBuffers.size(); ++index) {
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        if (vkBeginCommandBuffer(resources.commandBuffers[index], &beginInfo) != VK_SUCCESS) return result;
        VkImageMemoryBarrier textureBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        textureBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        textureBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        textureBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        textureBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        textureBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        textureBarrier.image = resources.textureImage;
        textureBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        textureBarrier.subresourceRange.levelCount = 1;
        textureBarrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(resources.commandBuffers[index], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &textureBarrier);
        VkBufferImageCopy uploadRegion{};
        uploadRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        uploadRegion.imageSubresource.layerCount = 1;
        uploadRegion.imageExtent = {texture.width, texture.height, 1};
        vkCmdCopyBufferToImage(resources.commandBuffers[index], resources.uploadBuffer, resources.textureImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &uploadRegion);
        textureBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        textureBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        textureBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        textureBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(resources.commandBuffers[index], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &textureBarrier);
        VkClearValue clear{};
        clear.color.float32[0] = 0.02F;
        clear.color.float32[1] = 0.02F;
        clear.color.float32[2] = 0.02F;
        clear.color.float32[3] = 1.0F;
        VkRenderPassBeginInfo renderInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderInfo.renderPass = resources.renderPass;
        renderInfo.framebuffer = resources.framebuffers[index];
        renderInfo.renderArea.extent = resources.extent;
        renderInfo.clearValueCount = 1;
        renderInfo.pClearValues = &clear;
        vkCmdBeginRenderPass(resources.commandBuffers[index], &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(resources.commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, resources.pipeline);
        vkCmdBindDescriptorSets(resources.commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, resources.pipelineLayout, 0, 1,
                                &resources.descriptorSet, 0, nullptr);
        vkCmdDraw(resources.commandBuffers[index], 3, 1, 0, 0);
        vkCmdEndRenderPass(resources.commandBuffers[index]);
        if (vkEndCommandBuffer(resources.commandBuffers[index]) != VK_SUCCESS) return result;
    }
    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vkCreateSemaphore(resources.device, &semaphoreInfo, nullptr, &resources.imageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(resources.device, &semaphoreInfo, nullptr, &resources.renderFinished) != VK_SUCCESS ||
        vkCreateFence(resources.device, &fenceInfo, nullptr, &resources.fence) != VK_SUCCESS) return result;
    uint32_t imageIndex = 0;
    const VkResult acquireResult = vkAcquireNextImageKHR(resources.device, resources.swapchain, 5'000'000'000ULL,
                                                         resources.imageAvailable, VK_NULL_HANDLE, &imageIndex);
    result.acquireAttempted = true;
    result.acquireDriverResult = static_cast<int32_t>(acquireResult);
    result.status = ClassifyDriverResult(result.acquireDriverResult);
    if (result.status != VulkanPresentStatus::None) return result;
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &resources.imageAvailable;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &resources.commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &resources.renderFinished;
    const VkResult submitResult = vkQueueSubmit(resources.queue, 1, &submitInfo, resources.fence);
    result.submitAttempted = true;
    result.submitDriverResult = static_cast<int32_t>(submitResult);
    result.status = ClassifyDriverResult(result.submitDriverResult);
    if (result.status != VulkanPresentStatus::None) return result;
    const VkResult fenceWaitResult = vkWaitForFences(resources.device, 1, &resources.fence, VK_TRUE, 5'000'000'000ULL);
    result.fenceWaitAttempted = true;
    result.fenceWaitDriverResult = static_cast<int32_t>(fenceWaitResult);
    result.status = ClassifyDriverResult(result.fenceWaitDriverResult);
    if (result.status != VulkanPresentStatus::None) return result;
    result.frameSubmitted = true;
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &resources.renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &resources.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    const VkResult presentResult = vkQueuePresentKHR(resources.queue, &presentInfo);
    result.presentAttempted = true;
    result.presentDriverResult = static_cast<int32_t>(presentResult);
    result.status = ClassifyDriverResult(result.presentDriverResult);
    if (result.status != VulkanPresentStatus::None) return result;
    result.framePresented = true;
    result.status = VulkanPresentStatus::Presented;
    return result;
}

} // namespace NeoEngine

// NEOENGINE_VULKAN_TEXTURED_PRESENT_CPP
