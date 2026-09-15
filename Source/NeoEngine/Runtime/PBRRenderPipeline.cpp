#include "Runtime/PBRRenderPipeline.h"

#include <utility>

namespace NeoEngine {

PBRRenderPipeline::~PBRRenderPipeline() { Destroy(); }

PBRRenderPipeline::PBRRenderPipeline(PBRRenderPipeline&& other) noexcept
    : device_(other.device_),
      materialLayout_(other.materialLayout_),
      lightingDescriptors_(std::move(other.lightingDescriptors_)),
      pipeline_(std::move(other.pipeline_)) {
    other.device_ = VK_NULL_HANDLE;
    other.materialLayout_ = VK_NULL_HANDLE;
}

PBRRenderPipeline& PBRRenderPipeline::operator=(PBRRenderPipeline&& other) noexcept {
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        materialLayout_ = other.materialLayout_;
        lightingDescriptors_ = std::move(other.lightingDescriptors_);
        pipeline_ = std::move(other.pipeline_);
        other.device_ = VK_NULL_HANDLE;
        other.materialLayout_ = VK_NULL_HANDLE;
    }
    return *this;
}

bool PBRRenderPipeline::Initialize(
    VkDevice device,
    VkRenderPass renderPass,
    VkDescriptorSetLayout materialLayout,
    const std::vector<uint32_t>& vertexSpv,
    const std::vector<uint32_t>& fragmentSpv,
    const VkVertexInputBindingDescription& vertexBinding,
    const std::vector<VkVertexInputAttributeDescription>& vertexAttributes) {
    if (device == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE ||
        materialLayout == VK_NULL_HANDLE || vertexSpv.empty() || fragmentSpv.empty()) {
        return false;
    }

    Destroy();
    device_ = device;
    materialLayout_ = materialLayout;

    // Matches pbr_lighting.frag set=1,binding=0 LightingFrame.
    const std::vector<DescriptorLayoutBindingInfo> lightingBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}
    };
    if (!lightingDescriptors_.Initialize(device_, lightingBindings, 16)) {
        Destroy();
        return false;
    }

    VulkanPipelineConfig config{};
    config.renderPass = renderPass;
    config.subpass = 0;
    config.vertexSpv = vertexSpv;
    config.fragmentSpv = fragmentSpv;
    config.vertexBinding = vertexBinding;
    config.vertexAttributes = vertexAttributes;
    config.cullMode = VK_CULL_MODE_BACK_BIT;
    config.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.depthTestEnable = true;
    config.depthWriteEnable = true;
    config.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    config.blendEnable = false;
    config.descriptorSetLayouts = {materialLayout_, lightingDescriptors_.GetLayout()};

    // neo_mesh.vert consumes a mat4 transform.mvp push constant.
    VkPushConstantRange transformRange{};
    transformRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    transformRange.offset = 0;
    transformRange.size = sizeof(float) * 16;
    config.pushConstantRanges = {transformRange};

    if (!pipeline_.Initialize(device_, config)) {
        Destroy();
        return false;
    }

    return true;
}

VkDescriptorSet PBRRenderPipeline::AllocateLightingSet() {
    if (!lightingDescriptors_.IsValid()) return VK_NULL_HANDLE;
    return lightingDescriptors_.AllocateSet();
}

void PBRRenderPipeline::UpdateLightingBuffer(VkDescriptorSet descriptorSet,
                                              VkBuffer buffer,
                                              VkDeviceSize offset,
                                              VkDeviceSize range) {
    if (!lightingDescriptors_.IsValid() || descriptorSet == VK_NULL_HANDLE ||
        buffer == VK_NULL_HANDLE || range == 0) {
        return;
    }
    lightingDescriptors_.UpdateBufferBinding(descriptorSet, 0,
                                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              buffer, offset, range);
}

void PBRRenderPipeline::Bind(VkCommandBuffer commandBuffer,
                             VkDescriptorSet materialSet,
                             VkDescriptorSet lightingSet) const {
    if (!IsValid() || commandBuffer == VK_NULL_HANDLE ||
        materialSet == VK_NULL_HANDLE || lightingSet == VK_NULL_HANDLE) {
        return;
    }

    const VkDescriptorSet sets[2] = {materialSet, lightingSet};
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_.GetPipelineLayout(), 0, 2, sets, 0, nullptr);
}

void PBRRenderPipeline::Destroy() {
    pipeline_.Destroy();
    lightingDescriptors_.Destroy();
    device_ = VK_NULL_HANDLE;
    materialLayout_ = VK_NULL_HANDLE;
}

} // namespace NeoEngine
