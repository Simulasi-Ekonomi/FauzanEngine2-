#include "Animation/SkinningGPU.h"
#include "Runtime/VulkanContext.h"

#include <cstdio>
#include <vector>

int main() {
    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::puts("SKINNING_GPU_SMOKE_SKIPPED_NO_VULKAN");
        return 0;
    }

    SkinningGPU skinning;
    if (!skinning.Initialize(context.Device(), context.PhysicalDevice()) || !skinning.IsValid()) return 1;
    if (!skinning.UploadBones({}) || skinning.UploadedBoneCount() != 0U) return 2;
    if (skinning.UploadBones(std::vector<SkinningGPU::BoneMatrix>(SkinningGPU::MaxBones + 1U))) return 3;
    auto invalid = SkinningGPU::BoneMatrix{};
    invalid.fill(0.0F);
    invalid[0] = 1.0F;
    invalid[5] = 1.0F;
    invalid[10] = 1.0F;
    invalid[15] = 1.0F;
    invalid[3] = __builtin_nanf("");
    if (skinning.UploadBones({invalid})) return 4;
    auto invalid = SkinningGPU::BoneMatrix{};
    invalid.fill(0.0F);
    invalid[0] = 1.0F;
    invalid[5] = 1.0F;
    invalid[10] = 1.0F;
    invalid[15] = 1.0F;
    invalid[3] = __builtin_nanf("");
    if (skinning.UploadBones({invalid})) return 4;

    std::vector<SkinningGPU::BoneMatrix> palette(2);
    for (auto& matrix : palette) {
        matrix.fill(0.0F);
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0F;
    }
    palette[1][12] = 1.0F;
    if (!skinning.UploadBones(palette) || skinning.UploadedBoneCount() != 2U || !skinning.IsValid()) return 5;
    if (skinning.GetBoneBuffer() == VK_NULL_HANDLE || skinning.GetDescriptorSet() == VK_NULL_HANDLE ||
        skinning.GetDescriptorSetLayout() == VK_NULL_HANDLE) return 5;

    skinning.Destroy();
    if (skinning.IsValid() || skinning.GetBoneBuffer() != VK_NULL_HANDLE || skinning.UploadedBoneCount() != 0U) return 7;
    context.Reset();
    std::puts("SKINNING_GPU_SMOKE_OK init=1 upload=1 descriptor=1 destroy=1");
    return 0;
}
