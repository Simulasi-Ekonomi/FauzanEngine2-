#include "VulkanOffscreen.h"

#include "VulkanContext.h"

#include <array>
#include <fstream>
#include <limits>
#include <vector>

namespace NeoEngine {
namespace {

constexpr VkFormat kColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;

std::vector<uint32_t> ReadSpirv(const char* path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return {};
    }
    const std::streamsize size = stream.tellg();
    if (size <= 0 || (size % static_cast<std::streamsize>(sizeof(uint32_t))) != 0) {
        return {};
    }
    std::vector<uint32_t> words(static_cast<size_t>(size) / sizeof(uint32_t));
    stream.seekg(0);
    if (!stream.read(reinterpret_cast<char*>(words.data()), size)) {
        return {};
    }
    return words;
}

uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeBits, VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(device, &properties);
    for (uint32_t index = 0; index < properties.memoryTypeCount; ++index) {
        const bool supported = (typeBits & (1U << index)) != 0;
        const bool matches = (properties.memoryTypes[index].propertyFlags & required) == required;
        if (supported && matches) {
            return index;
        }
    }
    return UINT32_MAX;
}

uint64_t HashBytes(const uint8_t* bytes, size_t count) {
    uint64_t hash = kHashOffset;
    for (size_t index = 0; index < count; ++index) {
        hash ^= bytes[index];
        hash *= kHashPrime;
    }
    return hash;
}

struct OffscreenResources {
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkImage colorImage = VK_NULL_HANDLE;
    VkDeviceMemory colorMemory = VK_NULL_HANDLE;
    VkBuffer readbackBuffer = VK_NULL_HANDLE;
    VkDeviceMemory readbackMemory = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkImageView colorView = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    ~OffscreenResources() {
        if (device == VK_NULL_HANDLE) {
            return;
        }
        vkDeviceWaitIdle(device);
        if (fence != VK_NULL_HANDLE) vkDestroyFence(device, fence, nullptr);
        if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, pipeline, nullptr);
        if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (colorView != VK_NULL_HANDLE) vkDestroyImageView(device, colorView, nullptr);
        if (renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(device, renderPass, nullptr);
        if (readbackBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, readbackBuffer, nullptr);
        if (readbackMemory != VK_NULL_HANDLE) vkFreeMemory(device, readbackMemory, nullptr);
        if (colorImage != VK_NULL_HANDLE) vkDestroyImage(device, colorImage, nullptr);
        if (colorMemory != VK_NULL_HANDLE) vkFreeMemory(device, colorMemory, nullptr);
        if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, commandPool, nullptr);
    }
};

} // namespace

VulkanOffscreenResult VulkanOffscreenRenderer::RenderTriangle(uint32_t width, uint32_t height) {
    VulkanOffscreenResult result{};
    if (width == 0 || height == 0 || width > 2048 || height > 2048) {
        return result;
    }

    VulkanContext context;
    if (!context.Initialize()) {
        return result;
    }
    result.deviceCreated = true;
    result.width = width;
    result.height = height;

    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(context.PhysicalDevice(), kColorFormat, &formatProperties);
    const VkFormatFeatureFlags requiredFeatures = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if ((formatProperties.optimalTilingFeatures & requiredFeatures) != requiredFeatures) {
        return result;
    }

    const uint64_t byteCount64 = static_cast<uint64_t>(width) * height * 4U;
    if (byteCount64 > std::numeric_limits<VkDeviceSize>::max() || byteCount64 > (64ULL * 1024ULL * 1024ULL)) {
        return result;
    }
    const VkDeviceSize byteCount = static_cast<VkDeviceSize>(byteCount64);

    OffscreenResources resources{};
    resources.device = context.Device();

    VkCommandPoolCreateInfo commandPoolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolInfo.queueFamilyIndex = context.GraphicsQueueFamily();
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(resources.device, &commandPoolInfo, nullptr, &resources.commandPool) != VK_SUCCESS) {
        return result;
    }

    VkCommandBufferAllocateInfo commandBufferInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferInfo.commandPool = resources.commandPool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(resources.device, &commandBufferInfo, &resources.commandBuffer) != VK_SUCCESS) {
        return result;
    }

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = kColorFormat;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(resources.device, &imageInfo, nullptr, &resources.colorImage) != VK_SUCCESS) {
        return result;
    }

    VkMemoryRequirements imageRequirements{};
    vkGetImageMemoryRequirements(resources.device, resources.colorImage, &imageRequirements);
    const uint32_t imageMemoryType = FindMemoryType(
        context.PhysicalDevice(), imageRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (imageMemoryType == UINT32_MAX) {
        return result;
    }
    VkMemoryAllocateInfo imageAllocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    imageAllocation.allocationSize = imageRequirements.size;
    imageAllocation.memoryTypeIndex = imageMemoryType;
    if (vkAllocateMemory(resources.device, &imageAllocation, nullptr, &resources.colorMemory) != VK_SUCCESS ||
        vkBindImageMemory(resources.device, resources.colorImage, resources.colorMemory, 0) != VK_SUCCESS) {
        return result;
    }

    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = byteCount;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(resources.device, &bufferInfo, nullptr, &resources.readbackBuffer) != VK_SUCCESS) {
        return result;
    }
    VkMemoryRequirements bufferRequirements{};
    vkGetBufferMemoryRequirements(resources.device, resources.readbackBuffer, &bufferRequirements);
    const uint32_t bufferMemoryType = FindMemoryType(
        context.PhysicalDevice(),
        bufferRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (bufferMemoryType == UINT32_MAX) {
        return result;
    }
    VkMemoryAllocateInfo bufferAllocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    bufferAllocation.allocationSize = bufferRequirements.size;
    bufferAllocation.memoryTypeIndex = bufferMemoryType;
    if (vkAllocateMemory(resources.device, &bufferAllocation, nullptr, &resources.readbackMemory) != VK_SUCCESS ||
        vkBindBufferMemory(resources.device, resources.readbackBuffer, resources.readbackMemory, 0) != VK_SUCCESS) {
        return result;
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = kColorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = 0;
    dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependency.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(resources.device, &renderPassInfo, nullptr, &resources.renderPass) != VK_SUCCESS) {
        return result;
    }

    VkImageViewCreateInfo imageViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewInfo.image = resources.colorImage;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = kColorFormat;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(resources.device, &imageViewInfo, nullptr, &resources.colorView) != VK_SUCCESS) {
        return result;
    }

    VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = resources.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &resources.colorView;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;
    if (vkCreateFramebuffer(resources.device, &framebufferInfo, nullptr, &resources.framebuffer) != VK_SUCCESS) {
        return result;
    }

    const std::vector<uint32_t> vertexCode = ReadSpirv(NEO_SHADER_DIR "/neo_triangle.vert.spv");
    const std::vector<uint32_t> fragmentCode = ReadSpirv(NEO_SHADER_DIR "/neo_triangle.frag.spv");
    if (vertexCode.empty() || fragmentCode.empty()) {
        return result;
    }

    VkShaderModuleCreateInfo shaderInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderInfo.codeSize = vertexCode.size() * sizeof(uint32_t);
    shaderInfo.pCode = vertexCode.data();
    VkShaderModule vertexModule = VK_NULL_HANDLE;
    VkShaderModule fragmentModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(resources.device, &shaderInfo, nullptr, &vertexModule) != VK_SUCCESS) {
        return result;
    }
    shaderInfo.codeSize = fragmentCode.size() * sizeof(uint32_t);
    shaderInfo.pCode = fragmentCode.data();
    if (vkCreateShaderModule(resources.device, &shaderInfo, nullptr, &fragmentModule) != VK_SUCCESS) {
        vkDestroyShaderModule(resources.device, vertexModule, nullptr);
        return result;
    }

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentModule;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.maxDepth = 1.0F;
    VkRect2D scissor{};
    scissor.extent = {width, height};
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

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if (vkCreatePipelineLayout(resources.device, &pipelineLayoutInfo, nullptr, &resources.pipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(resources.device, fragmentModule, nullptr);
        vkDestroyShaderModule(resources.device, vertexModule, nullptr);
        return result;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterization;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = resources.pipelineLayout;
    pipelineInfo.renderPass = resources.renderPass;
    pipelineInfo.subpass = 0;
    const VkResult pipelineResult = vkCreateGraphicsPipelines(
        resources.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resources.pipeline);
    vkDestroyShaderModule(resources.device, fragmentModule, nullptr);
    vkDestroyShaderModule(resources.device, vertexModule, nullptr);
    if (pipelineResult != VK_SUCCESS) {
        return result;
    }
    result.pipelineCreated = true;

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(resources.commandBuffer, &beginInfo) != VK_SUCCESS) {
        return result;
    }
    VkClearValue clearValue{};
    VkRenderPassBeginInfo renderBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderBegin.renderPass = resources.renderPass;
    renderBegin.framebuffer = resources.framebuffer;
    renderBegin.renderArea.extent = {width, height};
    renderBegin.clearValueCount = 1;
    renderBegin.pClearValues = &clearValue;
    vkCmdBeginRenderPass(resources.commandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(resources.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, resources.pipeline);
    vkCmdDraw(resources.commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(resources.commandBuffer);

    VkBufferImageCopy copyRegion{};
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(
        resources.commandBuffer,
        resources.colorImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        resources.readbackBuffer,
        1,
        &copyRegion);

    VkBufferMemoryBarrier readbackBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    readbackBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    readbackBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    readbackBarrier.buffer = resources.readbackBuffer;
    readbackBarrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(
        resources.commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0,
        0,
        nullptr,
        1,
        &readbackBarrier,
        0,
        nullptr);

    if (vkEndCommandBuffer(resources.commandBuffer) != VK_SUCCESS) {
        return result;
    }
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vkCreateFence(resources.device, &fenceInfo, nullptr, &resources.fence) != VK_SUCCESS) {
        return result;
    }
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &resources.commandBuffer;
    if (vkQueueSubmit(context.GraphicsQueue(), 1, &submitInfo, resources.fence) != VK_SUCCESS ||
        vkWaitForFences(resources.device, 1, &resources.fence, VK_TRUE, 5'000'000'000ULL) != VK_SUCCESS) {
        return result;
    }
    result.commandSubmitted = true;

    void* mapped = nullptr;
    if (vkMapMemory(resources.device, resources.readbackMemory, 0, byteCount, 0, &mapped) != VK_SUCCESS || mapped == nullptr) {
        return result;
    }
    const auto* pixels = static_cast<const uint8_t*>(mapped);
    result.pixelHash = HashBytes(pixels, static_cast<size_t>(byteCount));
    for (VkDeviceSize offset = 0; offset < byteCount; offset += 4) {
        if (pixels[offset] != 0 || pixels[offset + 1] != 0 || pixels[offset + 2] != 0 || pixels[offset + 3] != 0) {
            ++result.nonClearPixelCount;
        }
    }
    vkUnmapMemory(resources.device, resources.readbackMemory);
    result.pixelsReadback = result.nonClearPixelCount > 0 && result.pixelHash != 0;
    return result;
}

} // namespace NeoEngine
