#pragma once
#include "Animation/Skeleton.h"
#include "Runtime/VulkanGPUBuffer.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
namespace NeoEngine {
class GPUSkinningPaletteBuffer {
public:
 static constexpr size_t kMaxBones = Skeleton::kMaxBones;
 static constexpr uint32_t kFramesInFlight = 2U;
 static constexpr uint32_t kMaxPalettesPerFrame = 128U;
 static constexpr VkDeviceSize kPaletteByteSize = sizeof(Mat4) * kMaxBones;
 bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice);
 [[nodiscard]] bool BeginFrame(uint32_t frameIndex) noexcept;
 bool UploadPalette(const std::vector<Mat4>& palette);
 void UseDefaultPalette() noexcept;
 void Destroy();
 [[nodiscard]] bool IsValid() const { return buffer_.IsValid(); }
 [[nodiscard]] VkBuffer GetBuffer() const { return buffer_.GetBuffer(); }
 [[nodiscard]] size_t BoneCount() const { return boneCount_; }
 [[nodiscard]] uint32_t DynamicOffset() const { return dynamicOffset_; }
 [[nodiscard]] VkDeviceSize PaletteStride() const { return paletteStride_; }
private:
 VulkanGPUBuffer buffer_;
 size_t boneCount_=0U;
 VkDeviceSize paletteStride_=0U;
 uint32_t frameIndex_=0U;
 uint32_t nextPaletteIndex_=1U;
 uint32_t dynamicOffset_=0U;
};
}
