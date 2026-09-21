#pragma once

#include "Runtime/VulkanGPUBuffer.h"
#include "Runtime/VulkanDescriptorManager.h"

#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <vector>

class SkinningGPU
{
public:
    static constexpr uint32_t MaxBones = 64U;
    using BoneMatrix = std::array<float, 16>;

    SkinningGPU() = default;
    ~SkinningGPU();

    SkinningGPU(const SkinningGPU&) = delete;
    SkinningGPU& operator=(const SkinningGPU&) = delete;
    SkinningGPU(SkinningGPU&& other) noexcept;
    SkinningGPU& operator=(SkinningGPU&& other) noexcept;

    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice);
    bool UploadBones(const std::vector<BoneMatrix>& matrices);
    bool Bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;
    void Destroy();

    [[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorManager_.GetLayout(); }
    [[nodiscard]] VkDescriptorSet GetDescriptorSet() const { return descriptorSet_; }
    [[nodiscard]] VkBuffer GetBoneBuffer() const { return boneBuffer_.GetBuffer(); }
    [[nodiscard]] bool IsValid() const { return boneBuffer_.IsValid() && descriptorManager_.IsValid() && descriptorSet_ != VK_NULL_HANDLE && uploadedBoneCount_ <= MaxBones; }
    [[nodiscard]] uint32_t UploadedBoneCount() const { return uploadedBoneCount_; }

private:
    static bool IsFiniteMatrix(const BoneMatrix& matrix);
    static std::array<BoneMatrix, MaxBones> IdentityPalette();

    NeoEngine::VulkanGPUBuffer boneBuffer_;
    NeoEngine::VulkanDescriptorManager descriptorManager_;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    uint32_t uploadedBoneCount_ = 0U;
};
