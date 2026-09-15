#pragma once

#include "Runtime/PBRIBL.h"
#include "Runtime/VulkanDescriptorManager.h"
#include "Runtime/VulkanGraphicsPipeline.h"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

namespace NeoEngine {

class PBRRenderPipeline {
public:
    PBRRenderPipeline() = default;
    ~PBRRenderPipeline();

    PBRRenderPipeline(const PBRRenderPipeline&) = delete;
    PBRRenderPipeline& operator=(const PBRRenderPipeline&) = delete;

    PBRRenderPipeline(PBRRenderPipeline&& other) noexcept;
    PBRRenderPipeline& operator=(PBRRenderPipeline&& other) noexcept;

    // The material descriptor layout is owned by PBRMaterial/VulkanDescriptorManager.
    // This class owns the frame-lighting layout and the graphics pipeline.
    bool Initialize(VkDevice device,
                    VkRenderPass renderPass,
                    VkDescriptorSetLayout materialLayout,
                    const std::vector<uint32_t>& vertexSpv,
                    const std::vector<uint32_t>& fragmentSpv,
                    const VkVertexInputBindingDescription& vertexBinding,
                    const std::vector<VkVertexInputAttributeDescription>& vertexAttributes);

    // Creates a pipeline whose fragment shader additionally consumes an IBL
    // descriptor set at set=2. The existing Initialize/Bind API remains intact.
    bool InitializeWithIBL(VkDevice device,
                           VkRenderPass renderPass,
                           VkDescriptorSetLayout materialLayout,
                           VkDescriptorSetLayout iblLayout,
                           const std::vector<uint32_t>& vertexSpv,
                           const std::vector<uint32_t>& fragmentSpv,
                           const VkVertexInputBindingDescription& vertexBinding,
                           const std::vector<VkVertexInputAttributeDescription>& vertexAttributes);

    VkDescriptorSet AllocateLightingSet();

    void UpdateLightingBuffer(VkDescriptorSet descriptorSet,
                              VkBuffer buffer,
                              VkDeviceSize offset,
                              VkDeviceSize range);

    void Bind(VkCommandBuffer commandBuffer,
              VkDescriptorSet materialSet,
              VkDescriptorSet lightingSet) const;

    // Backward-compatible IBL bind using default PBR IBL settings.
    void BindWithIBL(VkCommandBuffer commandBuffer,
                     VkDescriptorSet materialSet,
                     VkDescriptorSet lightingSet,
                     VkDescriptorSet iblSet) const;

    // Preferred IBL bind path: supplies the runtime environment settings
    // without adding another descriptor allocation/update to the render loop.
    void BindWithIBL(VkCommandBuffer commandBuffer,
                     VkDescriptorSet materialSet,
                     VkDescriptorSet lightingSet,
                     VkDescriptorSet iblSet,
                     const PBRIBLSettings& settings) const;

    void Destroy();

    [[nodiscard]] VkDescriptorSetLayout GetLightingLayout() const { return lightingDescriptors_.GetLayout(); }
    [[nodiscard]] VkDescriptorSetLayout GetMaterialLayout() const { return materialLayout_; }
    [[nodiscard]] VkDescriptorSetLayout GetIBLLayout() const { return iblLayout_; }
    [[nodiscard]] VkPipeline GetPipeline() const { return pipeline_.GetPipeline(); }
    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return pipeline_.GetPipelineLayout(); }
    [[nodiscard]] bool HasIBL() const { return iblLayout_ != VK_NULL_HANDLE; }
    [[nodiscard]] bool IsValid() const { return pipeline_.IsValid() && lightingDescriptors_.IsValid() && materialLayout_ != VK_NULL_HANDLE; }

private:
    bool InitializeInternal(VkDevice device,
                            VkRenderPass renderPass,
                            VkDescriptorSetLayout materialLayout,
                            VkDescriptorSetLayout iblLayout,
                            const std::vector<uint32_t>& vertexSpv,
                            const std::vector<uint32_t>& fragmentSpv,
                            const VkVertexInputBindingDescription& vertexBinding,
                            const std::vector<VkVertexInputAttributeDescription>& vertexAttributes);

    VkDevice device_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout materialLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout iblLayout_ = VK_NULL_HANDLE;
    VulkanDescriptorManager lightingDescriptors_;
    VulkanGraphicsPipeline pipeline_;
};

} // namespace NeoEngine
