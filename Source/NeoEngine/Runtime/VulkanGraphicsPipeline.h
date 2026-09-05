#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NeoEngine {

struct VulkanPipelineConfig {
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;

    // Shader SPIR-V Bytes
    std::vector<uint32_t> vertexSpv;
    std::vector<uint32_t> fragmentSpv;

    // Vertex Layout
    VkVertexInputBindingDescription vertexBinding{};
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

    // Fixed Function States
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    bool blendEnable = false;

    // Pipeline Layout Descriptor Sets
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
};

class VulkanGraphicsPipeline {
public:
    VulkanGraphicsPipeline() = default;
    ~VulkanGraphicsPipeline();

    VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
    VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

    VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept;
    VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&& other) noexcept;

    bool Initialize(VkDevice device, const VulkanPipelineConfig& config);
    void Destroy();

    [[nodiscard]] VkPipeline GetPipeline() const { return pipeline_; }
    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return pipelineLayout_; }
    [[nodiscard]] bool IsValid() const { return pipeline_ != VK_NULL_HANDLE && pipelineLayout_ != VK_NULL_HANDLE; }

    static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<uint32_t>& code);

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
};

} // namespace NeoEngine
