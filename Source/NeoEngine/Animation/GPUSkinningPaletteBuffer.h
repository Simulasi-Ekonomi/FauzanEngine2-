#pragma once
#include "Animation/Skeleton.h"
#include "Runtime/VulkanGPUBuffer.h"
#include <cstddef>
#include <vector>
#include <vulkan/vulkan.h>
namespace NeoEngine {
class GPUSkinningPaletteBuffer {
public:
 static constexpr size_t kMaxBones = Skeleton::kMaxBones;
 bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice);
 bool UploadPalette(const std::vector<Mat4>& palette);
 void Destroy();
 [[nodiscard]] bool IsValid() const { return buffer_.IsValid(); }
 [[nodiscard]] VkBuffer GetBuffer() const { return buffer_.GetBuffer(); }
 [[nodiscard]] size_t BoneCount() const { return boneCount_; }
private:
 VulkanGPUBuffer buffer_;
 size_t boneCount_=0U;
};
}
