#include "Runtime/VulkanGraphicsPipeline.h"
#include "Runtime/VulkanGPUBuffer.h"
#include "Runtime/VulkanMeshBatchBuffer.h"
#include "Renderer/GPUDrivenRenderer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace {

constexpr VkFormat kColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr uint32_t kWidth = 128;
constexpr uint32_t kHeight = 128;

struct Resources {
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkBuffer readback = VK_NULL_HANDLE;
    VkDeviceMemory readbackMemory = VK_NULL_HANDLE;

    ~Resources() {
        if (device == VK_NULL_HANDLE) return;
        vkDeviceWaitIdle(device);
        if (fence) vkDestroyFence(device, fence, nullptr);
        if (framebuffer) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (view) vkDestroyImageView(device, view, nullptr);
        if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
        if (readback) vkDestroyBuffer(device, readback, nullptr);
        if (readbackMemory) vkFreeMemory(device, readbackMemory, nullptr);
        if (image) vkDestroyImage(device, image, nullptr);
        if (imageMemory) vkFreeMemory(device, imageMemory, nullptr);
        if (pool) vkDestroyCommandPool(device, pool, nullptr);
    }
};

uint32_t FindMemoryType(VkPhysicalDevice physical, uint32_t bits, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical, &properties);
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((bits & (1u << i)) != 0 &&
            (properties.memoryTypes[i].propertyFlags & flags) == flags) return i;
    }
    return UINT32_MAX;
}

std::vector<uint32_t> ReadSpv(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    const auto size = file.tellg();
    if (size <= 0 || size % static_cast<std::streamoff>(sizeof(uint32_t)) != 0) return {};
    std::vector<uint32_t> code(static_cast<size_t>(size) / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), size);
    return code;
}

NeoEngine::MeshVertex3D Vertex(float x, float y) {
    return {{x, y, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
}

} // namespace

int main() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FauzanEngine2 R3 real GPU indirect batch smoke";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &app;

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) return 1;

    uint32_t physicalCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr) != VK_SUCCESS || physicalCount == 0) {
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    std::vector<VkPhysicalDevice> physicals(physicalCount);
    vkEnumeratePhysicalDevices(instance, &physicalCount, physicals.data());
    VkPhysicalDevice physical = physicals.front();

    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, families.data());
    uint32_t graphicsFamily = UINT32_MAX;
    for (uint32_t i = 0; i < familyCount; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            graphicsFamily = i;
            break;
        }
    }
    if (graphicsFamily == UINT32_MAX) {
        vkDestroyInstance(instance, nullptr);
        return 1;
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = graphicsFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;

    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(physical, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, graphicsFamily, 0, &queue);
    Resources r{};
    r.device = device;

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &r.pool) != VK_SUCCESS) return 1;
    VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = r.pool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(device, &commandInfo, &r.commandBuffer) != VK_SUCCESS) return 1;

    const VkDeviceSize byteCount = static_cast<VkDeviceSize>(kWidth) * kHeight * 4;
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = kColorFormat;
    imageInfo.extent = {kWidth, kHeight, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageInfo, nullptr, &r.image) != VK_SUCCESS) return 1;
    VkMemoryRequirements imageReq{};
    vkGetImageMemoryRequirements(device, r.image, &imageReq);
    const uint32_t imageType = FindMemoryType(physical, imageReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (imageType == UINT32_MAX) return 1;
    VkMemoryAllocateInfo imageAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    imageAlloc.allocationSize = imageReq.size;
    imageAlloc.memoryTypeIndex = imageType;
    if (vkAllocateMemory(device, &imageAlloc, nullptr, &r.imageMemory) != VK_SUCCESS ||
        vkBindImageMemory(device, r.image, r.imageMemory, 0) != VK_SUCCESS) return 1;

    VkBufferCreateInfo readbackInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    readbackInfo.size = byteCount;
    readbackInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    readbackInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &readbackInfo, nullptr, &r.readback) != VK_SUCCESS) return 1;
    VkMemoryRequirements readbackReq{};
    vkGetBufferMemoryRequirements(device, r.readback, &readbackReq);
    const uint32_t readbackType = FindMemoryType(physical, readbackReq.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (readbackType == UINT32_MAX) return 1;
    VkMemoryAllocateInfo readbackAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    readbackAlloc.allocationSize = readbackReq.size;
    readbackAlloc.memoryTypeIndex = readbackType;
    if (vkAllocateMemory(device, &readbackAlloc, nullptr, &r.readbackMemory) != VK_SUCCESS ||
        vkBindBufferMemory(device, r.readback, r.readbackMemory, 0) != VK_SUCCESS) return 1;

    VkAttachmentDescription attachment{};
    attachment.format = kColorFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = 0;
    dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependency.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &r.renderPass) != VK_SUCCESS) return 1;

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = r.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = kColorFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &r.view) != VK_SUCCESS) return 1;
    VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = r.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &r.view;
    framebufferInfo.width = kWidth;
    framebufferInfo.height = kHeight;
    framebufferInfo.layers = 1;
    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &r.framebuffer) != VK_SUCCESS) return 1;

    NeoEngine::VulkanMeshBufferBuilder meshA;
    NeoEngine::VulkanMeshBufferBuilder meshB;
    const std::vector<NeoEngine::MeshVertex3D> a = {Vertex(-0.95f, -0.75f), Vertex(-0.10f, -0.75f), Vertex(-0.52f, 0.75f)};
    const std::vector<NeoEngine::MeshVertex3D> b = {Vertex(0.10f, -0.75f), Vertex(0.95f, -0.75f), Vertex(0.52f, 0.75f)};
    const std::vector<uint32_t> indices = {0, 1, 2};
    if (!meshA.BuildMesh(device, physical, a, indices) || !meshB.BuildMesh(device, physical, b, indices)) return 1;
    NeoEngine::VulkanMeshBatchBuffer batch;
    const std::vector<const NeoEngine::VulkanMeshBufferBuilder*> meshes = {&meshA, &meshB};
    if (!batch.Build(device, physical, meshes) || batch.MeshCount() != 2) return 1;
    if (batch.GetRange(0).vertexOffset != 0 || batch.GetRange(0).firstIndex != 0 ||
        batch.GetRange(1).vertexOffset != 3 || batch.GetRange(1).firstIndex != 3) return 1;

    struct Instance { float model[16]; } instances[2]{};
    for (auto& instance : instances) {
        instance.model[0] = instance.model[5] = instance.model[10] = instance.model[15] = 1.0f;
    }
    NeoEngine::VulkanGPUBuffer instanceBuffer;
    if (!instanceBuffer.Initialize(device, physical, sizeof(instances), NeoEngine::VulkanBufferType::VertexBuffer,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !instanceBuffer.UploadData(instances, sizeof(instances))) return 1;

    const auto vertexCode = ReadSpv(NEO_SHADER_DIR "/neo_mesh.vert.spv");
    const auto fragmentCode = ReadSpv(NEO_SHADER_DIR "/neo_mesh.frag.spv");
    if (vertexCode.empty() || fragmentCode.empty()) return 1;

    NeoEngine::VulkanPipelineConfig pipelineConfig{};
    pipelineConfig.renderPass = r.renderPass;
    pipelineConfig.vertexSpv = vertexCode;
    pipelineConfig.fragmentSpv = fragmentCode;
    pipelineConfig.cullMode = VK_CULL_MODE_NONE;
    pipelineConfig.depthTestEnable = false;
    pipelineConfig.depthWriteEnable = false;
    pipelineConfig.vertexBinding = {0, sizeof(NeoEngine::MeshVertex3D), VK_VERTEX_INPUT_RATE_VERTEX};
    pipelineConfig.vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, 24},
        {3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
        {4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16},
        {5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32},
        {6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48}
    };
    pipelineConfig.additionalVertexBindings = {{1, sizeof(Instance), VK_VERTEX_INPUT_RATE_INSTANCE}};
    pipelineConfig.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16}};
    NeoEngine::VulkanGraphicsPipeline pipeline;
    if (!pipeline.Initialize(device, pipelineConfig)) return 1;

    NeoEngine::GPUDrivenRenderer indirect;
    if (!indirect.Initialize(device, physical, 2)) return 1;
    if (!indirect.TrySubmitDraw({3, 1, 0, 0, 0}) || !indirect.TrySubmitDraw({3, 1, 3, 3, 1})) return 1;

    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(r.commandBuffer, &begin) != VK_SUCCESS) return 1;
    VkClearValue clear{};
    clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    VkRenderPassBeginInfo renderBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderBegin.renderPass = r.renderPass;
    renderBegin.framebuffer = r.framebuffer;
    renderBegin.renderArea.extent = {kWidth, kHeight};
    renderBegin.clearValueCount = 1;
    renderBegin.pClearValues = &clear;
    vkCmdBeginRenderPass(r.commandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport{0.0f, 0.0f, static_cast<float>(kWidth), static_cast<float>(kHeight), 0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, {kWidth, kHeight}};
    vkCmdSetViewport(r.commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(r.commandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(r.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipeline());
    const VkBuffer vertexBuffers[2] = {batch.GetVertexBuffer().GetBuffer(), instanceBuffer.GetBuffer()};
    const VkDeviceSize offsets[2] = {0, 0};
    vkCmdBindVertexBuffers(r.commandBuffer, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(r.commandBuffer, batch.GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    float identity[16]{};
    identity[0] = identity[5] = identity[10] = identity[15] = 1.0f;
    vkCmdPushConstants(r.commandBuffer, pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(identity), identity);
    if (!indirect.Execute(r.commandBuffer) || indirect.PendingDrawCount() != 0) return 1;
    vkCmdEndRenderPass(r.commandBuffer);

    VkBufferImageCopy copy{};
    copy.bufferRowLength = kWidth;
    copy.bufferImageHeight = kHeight;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.layerCount = 1;
    copy.imageExtent = {kWidth, kHeight, 1};
    vkCmdCopyImageToBuffer(r.commandBuffer, r.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, r.readback, 1, &copy);
    if (vkEndCommandBuffer(r.commandBuffer) != VK_SUCCESS) return 1;

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (vkCreateFence(device, &fenceInfo, nullptr, &r.fence) != VK_SUCCESS) return 1;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &r.commandBuffer;
    if (vkQueueSubmit(queue, 1, &submit, r.fence) != VK_SUCCESS ||
        vkWaitForFences(device, 1, &r.fence, VK_TRUE, std::numeric_limits<uint64_t>::max()) != VK_SUCCESS) return 1;

    void* mapped = nullptr;
    if (vkMapMemory(device, r.readbackMemory, 0, byteCount, 0, &mapped) != VK_SUCCESS) return 1;
    const auto* pixels = static_cast<const uint8_t*>(mapped);
    uint32_t nonClear = 0;
    for (VkDeviceSize i = 0; i < byteCount; i += 4) {
        if (pixels[i] != 0 || pixels[i + 1] != 0 || pixels[i + 2] != 0) ++nonClear;
    }
    vkUnmapMemory(device, r.readbackMemory);

    if (nonClear == 0) return 1;
    std::printf("R3_GPU_INDIRECT_OK draws=2 non_clear_pixels=%u\n", nonClear);

    pipeline.Destroy();
    instanceBuffer.Destroy();
    indirect.Destroy();
    batch.Destroy();
    meshA.Destroy();
    meshB.Destroy();
    vkDestroyDevice(device, nullptr);
    r.device = VK_NULL_HANDLE;
    vkDestroyInstance(instance, nullptr);
    return 0;
}
