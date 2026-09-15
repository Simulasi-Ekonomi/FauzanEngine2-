#include "Runtime/PBRRenderPipeline.h"

#include <algorithm>
#include <utility>

namespace NeoEngine {

namespace {
struct PBRIBLPushConstants {
    float transformPadding[16]{};
    float environmentIntensity = 1.0f;
    float irradianceStrength = 1.0f;
    float maxReflectionLod = 5.0f;
    float padding = 0.0f;
};
static_assert(sizeof(PBRIBLPushConstants) == 80, "PBR IBL push constant layout must remain 80 bytes");
}

PBRRenderPipeline::~PBRRenderPipeline() { Destroy(); }

PBRRenderPipeline::PBRRenderPipeline(PBRRenderPipeline&& other) noexcept
    : device_(other.device_), materialLayout_(other.materialLayout_), iblLayout_(other.iblLayout_), lightingDescriptors_(std::move(other.lightingDescriptors_)), pipeline_(std::move(other.pipeline_)) {
    other.device_ = VK_NULL_HANDLE;
    other.materialLayout_ = VK_NULL_HANDLE;
    other.iblLayout_ = VK_NULL_HANDLE;
}

PBRRenderPipeline& PBRRenderPipeline::operator=(PBRRenderPipeline&& other) noexcept {
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        materialLayout_ = other.materialLayout_;
        iblLayout_ = other.iblLayout_;
        lightingDescriptors_ = std::move(other.lightingDescriptors_);
        pipeline_ = std::move(other.pipeline_);
        other.device_ = VK_NULL_HANDLE;
        other.materialLayout_ = VK_NULL_HANDLE;
        other.iblLayout_ = VK_NULL_HANDLE;
    }
    return *this;
}

bool PBRRenderPipeline::Initialize(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout materialLayout,
                                   const std::vector<uint32_t>& vertexSpv, const std::vector<uint32_t>& fragmentSpv,
                                   const VkVertexInputBindingDescription& vertexBinding,
                                   const std::vector<VkVertexInputAttributeDescription>& vertexAttributes) {
    return InitializeInternal(device, renderPass, materialLayout, VK_NULL_HANDLE, vertexSpv, fragmentSpv,
                              vertexBinding, vertexAttributes);
}

bool PBRRenderPipeline::InitializeWithIBL(VkDevice device, VkRenderPass renderPass,
                                          VkDescriptorSetLayout materialLayout, VkDescriptorSetLayout iblLayout,
                                          const std::vector<uint32_t>& vertexSpv, const std::vector<uint32_t>& fragmentSpv,
                                          const VkVertexInputBindingDescription& vertexBinding,
                                          const std::vector<VkVertexInputAttributeDescription>& vertexAttributes) {
    if (iblLayout == VK_NULL_HANDLE) return false;
    return InitializeInternal(device, renderPass, materialLayout, iblLayout, vertexSpv, fragmentSpv,
                              vertexBinding, vertexAttributes);
}

bool PBRRenderPipeline::InitializeInternal(VkDevice device, VkRenderPass renderPass,
                                            VkDescriptorSetLayout materialLayout, VkDescriptorSetLayout iblLayout,
                                            const std::vector<uint32_t>& vertexSpv, const std::vector<uint32_t>& fragmentSpv,
                                            const VkVertexInputBindingDescription& vertexBinding,
                                            const std::vector<VkVertexInputAttributeDescription>& vertexAttributes) {
    if (device == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE || materialLayout == VK_NULL_HANDLE ||
        vertexSpv.empty() || fragmentSpv.empty()) return false;

    Destroy();
    device_ = device;
    materialLayout_ = materialLayout;
    iblLayout_ = iblLayout;

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
    if (iblLayout_ != VK_NULL_HANDLE) config.descriptorSetLayouts.push_back(iblLayout_);

    VkPushConstantRange transformRange{};
    transformRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    transformRange.offset = 0;
    transformRange.size = sizeof(float) * 16;

    VkPushConstantRange iblRange{};
    iblRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    iblRange.offset = sizeof(float) * 16;
    iblRange.size = sizeof(float) * 4;
    config.pushConstantRanges = {transformRange, iblRange};

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

void PBRRenderPipeline::UpdateLightingBuffer(VkDescriptorSet descriptorSet, VkBuffer buffer,
                                              VkDeviceSize offset, VkDeviceSize range) {
    if (!lightingDescriptors_.IsValid() || descriptorSet == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE || range == 0) return;
    lightingDescriptors_.UpdateBufferBinding(descriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer, offset, range);
}

void PBRRenderPipeline::Bind(VkCommandBuffer commandBuffer, VkDescriptorSet materialSet, VkDescriptorSet lightingSet) const {
    if (!IsValid() || HasIBL() || commandBuffer == VK_NULL_HANDLE || materialSet == VK_NULL_HANDLE || lightingSet == VK_NULL_HANDLE) return;
    const VkDescriptorSet sets[2] = {materialSet, lightingSet};
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetPipelineLayout(), 0, 2, sets, 0, nullptr);
}

void PBRRenderPipeline::BindWithIBL(VkCommandBuffer commandBuffer, VkDescriptorSet materialSet,
                                    VkDescriptorSet lightingSet, VkDescriptorSet iblSet) const {
    BindWithIBL(commandBuffer, materialSet, lightingSet, iblSet, PBRIBLSettings{});
}

void PBRRenderPipeline::BindWithIBL(VkCommandBuffer commandBuffer, VkDescriptorSet materialSet,
                                    VkDescriptorSet lightingSet, VkDescriptorSet iblSet,
                                    const PBRIBLSettings& settings) const {
    if (!IsValid() || !HasIBL() || commandBuffer == VK_NULL_HANDLE || materialSet == VK_NULL_HANDLE ||
        lightingSet == VK_NULL_HANDLE || iblSet == VK_NULL_HANDLE || !ValidatePBRIBLSettings(settings)) return;

    const VkDescriptorSet sets[3] = {materialSet, lightingSet, iblSet};
    PBRIBLPushConstants push{};
    push.environmentIntensity = settings.environmentIntensity;
    push.irradianceStrength = settings.irradianceStrength;
    push.maxReflectionLod = settings.maxReflectionLod;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.GetPipelineLayout(), 0, 3, sets, 0, nullptr);
    vkCmdPushConstants(commandBuffer, pipeline_.GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT,
                       sizeof(float) * 16, sizeof(float) * 4, &push.environmentIntensity);
}

void PBRRenderPipeline::Destroy() {
    pipeline_.Destroy();
    lightingDescriptors_.Destroy();
    device_ = VK_NULL_HANDLE;
    materialLayout_ = VK_NULL_HANDLE;
    iblLayout_ = VK_NULL_HANDLE;
}

} // namespace NeoEngine
