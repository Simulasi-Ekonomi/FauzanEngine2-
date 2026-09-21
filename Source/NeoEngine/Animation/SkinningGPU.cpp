#include "SkinningGPU.h"

#include <cmath>
#include <cstring>
#include <utility>

namespace {
constexpr VkDeviceSize kPaletteBytes =
    static_cast<VkDeviceSize>(SkinningGPU::MaxBones) * sizeof(SkinningGPU::BoneMatrix);
}

SkinningGPU::~SkinningGPU()
{
    Destroy();
}

SkinningGPU::SkinningGPU(SkinningGPU&& other) noexcept
    : boneBuffer_(std::move(other.boneBuffer_)),
      descriptorManager_(std::move(other.descriptorManager_)),
      descriptorSet_(other.descriptorSet_)
{
    other.descriptorSet_ = VK_NULL_HANDLE;
}

SkinningGPU& SkinningGPU::operator=(SkinningGPU&& other) noexcept
{
    if (this != &other) {
        Destroy();
        boneBuffer_ = std::move(other.boneBuffer_);
        descriptorManager_ = std::move(other.descriptorManager_);
        descriptorSet_ = other.descriptorSet_;
        other.descriptorSet_ = VK_NULL_HANDLE;
    }
    return *this;
}

bool SkinningGPU::Initialize(VkDevice device, VkPhysicalDevice physicalDevice)
{
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) return false;

    Destroy();

    if (!boneBuffer_.Initialize(device, physicalDevice, kPaletteBytes,
                                NeoEngine::VulkanBufferType::UniformBuffer,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        Destroy();
        return false;
    }

    const std::vector<NeoEngine::DescriptorLayoutBindingInfo> bindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1}
    };
    if (!descriptorManager_.Initialize(device, bindings, 1)) {
        Destroy();
        return false;
    }

    descriptorSet_ = descriptorManager_.AllocateSet();
    if (descriptorSet_ == VK_NULL_HANDLE) {
        Destroy();
        return false;
    }

    descriptorManager_.UpdateBufferBinding(
        descriptorSet_, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        boneBuffer_.GetBuffer(), 0, kPaletteBytes);

    return UploadBones({});
}

bool SkinningGPU::IsFiniteMatrix(const BoneMatrix& matrix)
{
    for (const float value : matrix) {
        if (!std::isfinite(value)) return false;
    }
    return true;
}

std::array<SkinningGPU::BoneMatrix, SkinningGPU::MaxBones> SkinningGPU::IdentityPalette()
{
    std::array<BoneMatrix, MaxBones> palette{};
    for (BoneMatrix& matrix : palette) {
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
    }
    return palette;
}

bool SkinningGPU::UploadBones(const std::vector<BoneMatrix>& matrices)
{
    if (!IsValid() || matrices.size() > MaxBones) return false;

    for (const BoneMatrix& matrix : matrices) {
        if (!IsFiniteMatrix(matrix)) return false;
    }

    std::array<BoneMatrix, MaxBones> palette = IdentityPalette();
    std::copy(matrices.begin(), matrices.end(), palette.begin());
    return boneBuffer_.UploadData(palette.data(), kPaletteBytes);
}

bool SkinningGPU::Bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const
{
    if (!IsValid() || commandBuffer == VK_NULL_HANDLE || pipelineLayout == VK_NULL_HANDLE) return false;

    const VkDescriptorSet set = descriptorSet_;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &set, 0, nullptr);
    return true;
}

void SkinningGPU::Destroy()
{
    descriptorSet_ = VK_NULL_HANDLE;
    descriptorManager_.Destroy();
    boneBuffer_.Destroy();
}
