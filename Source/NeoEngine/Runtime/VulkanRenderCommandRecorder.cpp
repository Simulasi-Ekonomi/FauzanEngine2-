#include "Runtime/VulkanRenderCommandRecorder.h"
#include <utility>

namespace NeoEngine {

VulkanRenderCommandRecorder::~VulkanRenderCommandRecorder() {
    Destroy();
}

VulkanRenderCommandRecorder::VulkanRenderCommandRecorder(VulkanRenderCommandRecorder&& other) noexcept {
    device_ = other.device_;
    commandPool_ = other.commandPool_;
    commandBuffer_ = other.commandBuffer_;
    isRecording_ = other.isRecording_;

    other.device_ = VK_NULL_HANDLE;
    other.commandPool_ = VK_NULL_HANDLE;
    other.commandBuffer_ = VK_NULL_HANDLE;
    other.isRecording_ = false;
}

VulkanRenderCommandRecorder& VulkanRenderCommandRecorder::operator=(VulkanRenderCommandRecorder&& other) noexcept {
    if (this != &other) {
        Destroy();

        device_ = other.device_;
        commandPool_ = other.commandPool_;
        commandBuffer_ = other.commandBuffer_;
        isRecording_ = other.isRecording_;

        other.device_ = VK_NULL_HANDLE;
        other.commandPool_ = VK_NULL_HANDLE;
        other.commandBuffer_ = VK_NULL_HANDLE;
        other.isRecording_ = false;
    }
    return *this;
}

bool VulkanRenderCommandRecorder::Initialize(VkDevice device, VkCommandPool commandPool) {
    if (device == VK_NULL_HANDLE || commandPool == VK_NULL_HANDLE) {
        return false;
    }

    Destroy();
    device_ = device;
    commandPool_ = commandPool;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    return true;
}

bool VulkanRenderCommandRecorder::BeginRecording() {
    if (!IsValid() || isRecording_) {
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
        return false;
    }

    isRecording_ = true;
    return true;
}

void VulkanRenderCommandRecorder::BeginRenderPass(VkRenderPass renderPass,
                                                 VkFramebuffer framebuffer,
                                                 VkExtent2D extent,
                                                 const VkClearValue* clearValues,
                                                 uint32_t clearValueCount) {
    if (!isRecording_ || renderPass == VK_NULL_HANDLE || framebuffer == VK_NULL_HANDLE) {
        return;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = clearValueCount;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderCommandRecorder::SetViewportAndScissor(uint32_t width, uint32_t height) {
    if (!isRecording_ || width == 0 || height == 0) {
        return;
    }

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {width, height};
    vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);
}

void VulkanRenderCommandRecorder::BindPipeline(VkPipeline pipeline) {
    if (!isRecording_ || pipeline == VK_NULL_HANDLE) {
        return;
    }
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void VulkanRenderCommandRecorder::BindDescriptorSets(VkPipelineLayout pipelineLayout,
                                                     const VkDescriptorSet* descriptorSets,
                                                     uint32_t descriptorSetCount) {
    if (!isRecording_ || pipelineLayout == VK_NULL_HANDLE || descriptorSets == nullptr || descriptorSetCount == 0) {
        return;
    }
    vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSetCount, descriptorSets, 0, nullptr);
}

void VulkanRenderCommandRecorder::BindVertexBuffer(VkBuffer vertexBuffer, VkDeviceSize offset) {
    if (!isRecording_ || vertexBuffer == VK_NULL_HANDLE) {
        return;
    }
    VkBuffer buffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { offset };
    vkCmdBindVertexBuffers(commandBuffer_, 0, 1, buffers, offsets);
}

void VulkanRenderCommandRecorder::BindIndexBuffer(VkBuffer indexBuffer, VkDeviceSize offset, VkIndexType indexType) {
    if (!isRecording_ || indexBuffer == VK_NULL_HANDLE) {
        return;
    }
    vkCmdBindIndexBuffer(commandBuffer_, indexBuffer, offset, indexType);
}

void VulkanRenderCommandRecorder::DrawIndexed(uint32_t indexCount,
                                             uint32_t instanceCount,
                                             uint32_t firstIndex,
                                             int32_t vertexOffset,
                                             uint32_t firstInstance) {
    if (!isRecording_ || indexCount == 0) {
        return;
    }
    vkCmdDrawIndexed(commandBuffer_, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanRenderCommandRecorder::EndRenderPass() {
    if (!isRecording_) {
        return;
    }
    vkCmdEndRenderPass(commandBuffer_);
}

bool VulkanRenderCommandRecorder::EndRecording() {
    if (!isRecording_) {
        return false;
    }

    if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS) {
        return false;
    }

    isRecording_ = false;
    return true;
}

void VulkanRenderCommandRecorder::Destroy() {
    if (device_ != VK_NULL_HANDLE && commandPool_ != VK_NULL_HANDLE && commandBuffer_ != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer_);
        commandBuffer_ = VK_NULL_HANDLE;
    }
    device_ = VK_NULL_HANDLE;
    commandPool_ = VK_NULL_HANDLE;
    isRecording_ = false;
}

} // namespace NeoEngine
