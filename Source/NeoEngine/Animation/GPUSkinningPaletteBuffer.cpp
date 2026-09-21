#include "GPUSkinningPaletteBuffer.h"
#include <limits>
#include <cstdint>
#include <cmath>
namespace NeoEngine {
bool GPUSkinningPaletteBuffer::Initialize(VkDevice device,VkPhysicalDevice physicalDevice) {
 if(device==VK_NULL_HANDLE||physicalDevice==VK_NULL_HANDLE) return false;
 return buffer_.Initialize(device,physicalDevice,sizeof(Mat4)*kMaxBones,VulkanBufferType::UniformBuffer,
   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}
bool GPUSkinningPaletteBuffer::UploadPalette(const std::vector<Mat4>& palette) {
 if(!buffer_.IsValid()||palette.empty()||palette.size()>kMaxBones) return false;
 for (const Mat4& matrix : palette) for (float value : matrix.m) if (!std::isfinite(value)) return false;
 constexpr size_t maxBytes = std::numeric_limits<VkDeviceSize>::max();
 if (palette.size() > maxBytes / sizeof(Mat4)) return false;
 const VkDeviceSize byteSize = static_cast<VkDeviceSize>(palette.size() * sizeof(Mat4));
 if(!buffer_.UploadData(palette.data(), byteSize)) return false;
 boneCount_=palette.size();
 return true;
}
void GPUSkinningPaletteBuffer::Destroy(){ buffer_.Destroy(); boneCount_=0U; }
}
