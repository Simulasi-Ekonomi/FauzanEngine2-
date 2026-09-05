#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NeoEngine {

class VulkanRenderCommandRecorder {
public:
    VulkanRenderCommandRecorder() = default;
    ~VulkanRenderCommandRecorder();

    VulkanRenderCommandRecorder(const VulkanRenderCommandRecorder&) = delete;
    VulkanRenderCommandRecorder& operator=(const VulkanRenderCommandRecorder&) = delete;

    VulkanRenderCommandRecorder(VulkanRenderCommandRecorder&& other) noexcept;
    VulkanRenderCommandRecorder& operator=(VulkanRenderCommandRecorder&& other) noexcept;

    bool Initialize(VkDevice device, VkCommandPool commandPool);

    bool BeginRecording();
    void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent, const VkClearValue* clearValues, uint32_t clearValueCount);
    void SetViewportAndScissor(uint32_t width, uint32_t height);
    void BindPipeline(VkPipeline pipeline);
    void BindDescriptorSets(VkPipelineLayout pipelineLayout, const VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount);
    void BindVertexBuffer(VkBuffer vertexBuffer, VkDeviceSize offset = 0);
    void BindIndexBuffer(VkBuffer indexBuffer, VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
    void EndRenderPass();
    bool EndRecording();

    void Destroy();

    [[nodiscard]] VkCommandBuffer GetCommandBuffer() const { return commandBuffer_; }
    [[nodiscard]] bool IsRecording() const { return isRecording_; }
    [[nodiscard]] bool IsValid() const { return commandBuffer_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    bool isRecording_ = false;
};

} // namespace NeoEngine
