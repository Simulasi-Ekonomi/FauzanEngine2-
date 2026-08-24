#include "VulkanTexturedOffscreen.h"

#include "VulkanContext.h"

#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace NeoEngine {
namespace {

constexpr VkFormat kFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;
constexpr VkDeviceSize kMaxBytes = 64ULL * 1024ULL * 1024ULL;

uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeBits, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(device, &properties);
    for (uint32_t index = 0; index < properties.memoryTypeCount; ++index) {
        if ((typeBits & (1U << index)) != 0 && (properties.memoryTypes[index].propertyFlags & flags) == flags) return index;
    }
    return UINT32_MAX;
}

uint64_t Hash(const uint8_t* bytes, size_t count) {
    uint64_t value = kHashOffset;
    for (size_t index = 0; index < count; ++index) { value ^= bytes[index]; value *= kHashPrime; }
    return value;
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

bool CreateHostBuffer(VkPhysicalDevice physical, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &info, nullptr, &buffer) != VK_SUCCESS) return false;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    const uint32_t type = FindMemoryType(physical, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (type == UINT32_MAX) return false;
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = type;
    return vkAllocateMemory(device, &allocation, nullptr, &memory) == VK_SUCCESS && vkBindBufferMemory(device, buffer, memory, 0) == VK_SUCCESS;
}

bool CreateDeviceImage(VkPhysicalDevice physical, VkDevice device, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = kFormat;
    info.extent = {width, height, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
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
    return vkAllocateMemory(device, &allocation, nullptr, &memory) == VK_SUCCESS && vkBindImageMemory(device, image, memory, 0) == VK_SUCCESS;
}

struct Resources {
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkBuffer upload = VK_NULL_HANDLE;
    VkDeviceMemory uploadMemory = VK_NULL_HANDLE;
    VkBuffer readback = VK_NULL_HANDLE;
    VkDeviceMemory readbackMemory = VK_NULL_HANDLE;
    VkImage texture = VK_NULL_HANDLE;
    VkDeviceMemory textureMemory = VK_NULL_HANDLE;
    VkImageView textureView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkImage color = VK_NULL_HANDLE;
    VkDeviceMemory colorMemory = VK_NULL_HANDLE;
    VkImageView colorView = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    ~Resources() {
        if (device == VK_NULL_HANDLE) return;
        vkDeviceWaitIdle(device);
        if (fence != VK_NULL_HANDLE) vkDestroyFence(device, fence, nullptr);
        if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, pipeline, nullptr);
        if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        if (setLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
        if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(device, renderPass, nullptr);
        if (colorView != VK_NULL_HANDLE) vkDestroyImageView(device, colorView, nullptr);
        if (sampler != VK_NULL_HANDLE) vkDestroySampler(device, sampler, nullptr);
        if (textureView != VK_NULL_HANDLE) vkDestroyImageView(device, textureView, nullptr);
        if (readback != VK_NULL_HANDLE) vkDestroyBuffer(device, readback, nullptr);
        if (readbackMemory != VK_NULL_HANDLE) vkFreeMemory(device, readbackMemory, nullptr);
        if (color != VK_NULL_HANDLE) vkDestroyImage(device, color, nullptr);
        if (colorMemory != VK_NULL_HANDLE) vkFreeMemory(device, colorMemory, nullptr);
        if (texture != VK_NULL_HANDLE) vkDestroyImage(device, texture, nullptr);
        if (textureMemory != VK_NULL_HANDLE) vkFreeMemory(device, textureMemory, nullptr);
        if (upload != VK_NULL_HANDLE) vkDestroyBuffer(device, upload, nullptr);
        if (uploadMemory != VK_NULL_HANDLE) vkFreeMemory(device, uploadMemory, nullptr);
        if (pool != VK_NULL_HANDLE) vkDestroyCommandPool(device, pool, nullptr);
    }
};

VkImageViewCreateInfo ViewInfo(VkImage image) {
    VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = kFormat;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.layerCount = 1;
    return info;
}

} // namespace

VulkanTexturedOffscreenResult VulkanTexturedOffscreenRenderer::Render(const RgbaTexture& texture, uint32_t width, uint32_t height) {
    VulkanTexturedOffscreenResult result{};
    const uint64_t textureBytes64 = static_cast<uint64_t>(texture.width) * texture.height * 4U;
    const uint64_t targetBytes64 = static_cast<uint64_t>(width) * height * 4U;
    if (width == 0 || height == 0 || width > 2048 || height > 2048 || texture.width == 0 || texture.height == 0 || textureBytes64 == 0 || textureBytes64 > kMaxBytes || targetBytes64 == 0 || targetBytes64 > kMaxBytes || texture.rgba.size() != textureBytes64) return result;
    VulkanContext context;
    if (!context.Initialize()) return result;
    result.deviceCreated = true;
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(context.PhysicalDevice(), kFormat, &properties);
    const VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if ((properties.optimalTilingFeatures & required) != required) return result;
    Resources resources{};
    resources.device = context.Device();
    const VkDeviceSize textureBytes = static_cast<VkDeviceSize>(textureBytes64);
    const VkDeviceSize targetBytes = static_cast<VkDeviceSize>(targetBytes64);

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = context.GraphicsQueueFamily();
    if (vkCreateCommandPool(resources.device, &poolInfo, nullptr, &resources.pool) != VK_SUCCESS) return result;
    VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = resources.pool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(resources.device, &commandInfo, &resources.command) != VK_SUCCESS ||
        !CreateHostBuffer(context.PhysicalDevice(), resources.device, textureBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, resources.upload, resources.uploadMemory) ||
        !CreateHostBuffer(context.PhysicalDevice(), resources.device, targetBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, resources.readback, resources.readbackMemory) ||
        !CreateDeviceImage(context.PhysicalDevice(), resources.device, texture.width, texture.height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, resources.texture, resources.textureMemory) ||
        !CreateDeviceImage(context.PhysicalDevice(), resources.device, width, height, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, resources.color, resources.colorMemory)) return result;
    void* uploadMapped = nullptr;
    if (vkMapMemory(resources.device, resources.uploadMemory, 0, textureBytes, 0, &uploadMapped) != VK_SUCCESS || uploadMapped == nullptr) return result;
    std::memcpy(uploadMapped, texture.rgba.data(), static_cast<size_t>(textureBytes));
    vkUnmapMemory(resources.device, resources.uploadMemory);
    const VkImageViewCreateInfo textureViewInfo = ViewInfo(resources.texture);
    const VkImageViewCreateInfo colorViewInfo = ViewInfo(resources.color);
    if (vkCreateImageView(resources.device, &textureViewInfo, nullptr, &resources.textureView) != VK_SUCCESS || vkCreateImageView(resources.device, &colorViewInfo, nullptr, &resources.colorView) != VK_SUCCESS) return result;
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

    VkAttachmentDescription attachment{};
    attachment.format = kFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference attachmentReference{};
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = 0;
    dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependency.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    passInfo.attachmentCount = 1;
    passInfo.pAttachments = &attachment;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpass;
    passInfo.dependencyCount = 1;
    passInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(resources.device, &passInfo, nullptr, &resources.renderPass) != VK_SUCCESS) return result;
    VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = resources.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &resources.colorView;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;
    if (vkCreateFramebuffer(resources.device, &framebufferInfo, nullptr, &resources.framebuffer) != VK_SUCCESS) return result;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo setLayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    setLayoutInfo.bindingCount = 1;
    setLayoutInfo.pBindings = &binding;
    if (vkCreateDescriptorSetLayout(resources.device, &setLayoutInfo, nullptr, &resources.setLayout) != VK_SUCCESS) return result;
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
    VkDescriptorPoolCreateInfo descriptorPoolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolInfo.maxSets = 1;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(resources.device, &descriptorPoolInfo, nullptr, &resources.descriptorPool) != VK_SUCCESS) return result;
    VkDescriptorSetAllocateInfo descriptorAllocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    descriptorAllocateInfo.descriptorPool = resources.descriptorPool;
    descriptorAllocateInfo.descriptorSetCount = 1;
    descriptorAllocateInfo.pSetLayouts = &resources.setLayout;
    if (vkAllocateDescriptorSets(resources.device, &descriptorAllocateInfo, &resources.descriptorSet) != VK_SUCCESS) return result;
    VkDescriptorImageInfo imageDescriptor{};
    imageDescriptor.sampler = resources.sampler;
    imageDescriptor.imageView = resources.textureView;
    imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = resources.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageDescriptor;
    vkUpdateDescriptorSets(resources.device, 1, &descriptorWrite, 0, nullptr);

    const std::vector<uint32_t> vertexCode = ReadSpirv(NEO_SHADER_DIR "/neo_texture.vert.spv");
    const std::vector<uint32_t> fragmentCode = ReadSpirv(NEO_SHADER_DIR "/neo_texture.frag.spv");
    if (vertexCode.empty() || fragmentCode.empty()) return result;
    VkShaderModuleCreateInfo shaderInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderInfo.codeSize = vertexCode.size() * sizeof(uint32_t);
    shaderInfo.pCode = vertexCode.data();
    VkShaderModule vertex = VK_NULL_HANDLE;
    VkShaderModule fragment = VK_NULL_HANDLE;
    if (vkCreateShaderModule(resources.device, &shaderInfo, nullptr, &vertex) != VK_SUCCESS) return result;
    shaderInfo.codeSize = fragmentCode.size() * sizeof(uint32_t);
    shaderInfo.pCode = fragmentCode.data();
    if (vkCreateShaderModule(resources.device, &shaderInfo, nullptr, &fragment) != VK_SUCCESS) { vkDestroyShaderModule(resources.device, vertex, nullptr); return result; }
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment;
    stages[1].pName = "main";
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
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &resources.setLayout;
    if (vkCreatePipelineLayout(resources.device, &pipelineLayoutInfo, nullptr, &resources.pipelineLayout) != VK_SUCCESS) { vkDestroyShaderModule(resources.device, fragment, nullptr); vkDestroyShaderModule(resources.device, vertex, nullptr); return result; }
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
    vkDestroyShaderModule(resources.device, fragment, nullptr);
    vkDestroyShaderModule(resources.device, vertex, nullptr);
    if (pipelineResult != VK_SUCCESS) return result;
    result.pipelineCreated = true;

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(resources.command, &beginInfo) != VK_SUCCESS) return result;
    VkImageMemoryBarrier textureBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    textureBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    textureBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    textureBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    textureBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    textureBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    textureBarrier.image = resources.texture;
    textureBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureBarrier.subresourceRange.levelCount = 1;
    textureBarrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(resources.command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &textureBarrier);
    VkBufferImageCopy uploadRegion{};
    uploadRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    uploadRegion.imageSubresource.layerCount = 1;
    uploadRegion.imageExtent = {texture.width, texture.height, 1};
    vkCmdCopyBufferToImage(resources.command, resources.upload, resources.texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &uploadRegion);
    textureBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    textureBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    textureBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    textureBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(resources.command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &textureBarrier);
    VkClearValue clear{};
    VkRenderPassBeginInfo renderInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderInfo.renderPass = resources.renderPass;
    renderInfo.framebuffer = resources.framebuffer;
    renderInfo.renderArea.extent = {width, height};
    renderInfo.clearValueCount = 1;
    renderInfo.pClearValues = &clear;
    vkCmdBeginRenderPass(resources.command, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(resources.command, VK_PIPELINE_BIND_POINT_GRAPHICS, resources.pipeline);
    vkCmdBindDescriptorSets(resources.command, VK_PIPELINE_BIND_POINT_GRAPHICS, resources.pipelineLayout, 0, 1, &resources.descriptorSet, 0, nullptr);
    vkCmdDraw(resources.command, 3, 1, 0, 0);
    vkCmdEndRenderPass(resources.command);
    VkBufferImageCopy copyRegion{};
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(resources.command, resources.color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, resources.readback, 1, &copyRegion);
    VkBufferMemoryBarrier readbackBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    readbackBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    readbackBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    readbackBarrier.buffer = resources.readback;
    readbackBarrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(resources.command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &readbackBarrier, 0, nullptr);
    if (vkEndCommandBuffer(resources.command) != VK_SUCCESS) return result;
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vkCreateFence(resources.device, &fenceInfo, nullptr, &resources.fence) != VK_SUCCESS) return result;
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &resources.command;
    if (vkQueueSubmit(context.GraphicsQueue(), 1, &submitInfo, resources.fence) != VK_SUCCESS || vkWaitForFences(resources.device, 1, &resources.fence, VK_TRUE, 5'000'000'000ULL) != VK_SUCCESS) return result;
    result.commandSubmitted = true;
    void* readbackMapped = nullptr;
    if (vkMapMemory(resources.device, resources.readbackMemory, 0, targetBytes, 0, &readbackMapped) != VK_SUCCESS || readbackMapped == nullptr) return result;
    const uint8_t* pixels = static_cast<const uint8_t*>(readbackMapped);
    result.pixelHash = Hash(pixels, static_cast<size_t>(targetBytes));
    for (VkDeviceSize offset = 0; offset < targetBytes; offset += 4) {
        if (pixels[offset] != 0 || pixels[offset + 1] != 0 || pixels[offset + 2] != 0) ++result.nonBlackPixelCount;
    }
    vkUnmapMemory(resources.device, resources.readbackMemory);
    result.pixelsReadback = result.pixelHash != 0;
    return result;
}

} // namespace NeoEngine
