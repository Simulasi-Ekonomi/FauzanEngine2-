#include "Runtime/VulkanGraphicsPipeline.h"
#include <utility>

namespace NeoEngine {

VulkanGraphicsPipeline::~VulkanGraphicsPipeline() { Destroy(); }

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept {
    device_ = other.device_; pipeline_ = other.pipeline_; pipelineLayout_ = other.pipelineLayout_;
    other.device_ = VK_NULL_HANDLE; other.pipeline_ = VK_NULL_HANDLE; other.pipelineLayout_ = VK_NULL_HANDLE;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::operator=(VulkanGraphicsPipeline&& other) noexcept {
    if (this != &other) {
        Destroy();
        device_ = other.device_; pipeline_ = other.pipeline_; pipelineLayout_ = other.pipelineLayout_;
        other.device_ = VK_NULL_HANDLE; other.pipeline_ = VK_NULL_HANDLE; other.pipelineLayout_ = VK_NULL_HANDLE;
    }
    return *this;
}

VkShaderModule VulkanGraphicsPipeline::CreateShaderModule(VkDevice device, const std::vector<uint32_t>& code) {
    if (device == VK_NULL_HANDLE || code.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    return vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS ? shaderModule : VK_NULL_HANDLE;
}

bool VulkanGraphicsPipeline::Initialize(VkDevice device, const VulkanPipelineConfig& config) {
    if (device == VK_NULL_HANDLE || config.renderPass == VK_NULL_HANDLE || config.vertexSpv.empty() || config.fragmentSpv.empty()) return false;
    Destroy();
    device_ = device;

    VkShaderModule vertShaderModule = CreateShaderModule(device_, config.vertexSpv);
    VkShaderModule fragShaderModule = CreateShaderModule(device_, config.fragmentSpv);
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        if (vertShaderModule != VK_NULL_HANDLE) vkDestroyShaderModule(device_, vertShaderModule, nullptr);
        if (fragShaderModule != VK_NULL_HANDLE) vkDestroyShaderModule(device_, fragShaderModule, nullptr);
        Destroy(); return false;
    }

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT; vertStage.module = vertShaderModule; vertStage.pName = "main";
    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT; fragStage.module = fragShaderModule; fragStage.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

    // Keep binding 0 exactly as before, then append optional R3 bindings.
    std::vector<VkVertexInputBindingDescription> bindings;
    if (config.vertexBinding.stride > 0) bindings.push_back(config.vertexBinding);
    bindings.insert(bindings.end(), config.additionalVertexBindings.begin(), config.additionalVertexBindings.end());

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = config.vertexAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = config.topology;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1; viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = config.cullMode; rasterizer.frontFace = config.frontFace;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = config.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = config.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = config.depthCompareOp;

    VkPipelineColorBlendAttachmentState colorAttachment{};
    colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (config.blendEnable) {
        colorAttachment.blendEnable = VK_TRUE;
        colorAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1; colorBlending.pAttachments = &colorAttachment;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(config.descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = config.descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size());
    layoutInfo.pPushConstantRanges = config.pushConstantRanges.data();
    if (vkCreatePipelineLayout(device_, &layoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, vertShaderModule, nullptr); vkDestroyShaderModule(device_, fragShaderModule, nullptr);
        Destroy(); return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo; pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState; pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling; pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending; pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_; pipelineInfo.renderPass = config.renderPass; pipelineInfo.subpass = config.subpass;

    const VkResult result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_);
    vkDestroyShaderModule(device_, vertShaderModule, nullptr); vkDestroyShaderModule(device_, fragShaderModule, nullptr);
    if (result != VK_SUCCESS) { Destroy(); return false; }
    return true;
}

void VulkanGraphicsPipeline::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        if (pipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_, pipeline_, nullptr); pipeline_ = VK_NULL_HANDLE; }
        if (pipelineLayout_ != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr); pipelineLayout_ = VK_NULL_HANDLE; }
        device_ = VK_NULL_HANDLE;
    }
}

} // namespace NeoEngine
