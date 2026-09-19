#include "GPUSkinningPaletteBuffer.h"
#include <limits>
namespace NeoEngine {
bool GPUSkinningPaletteBuffer::Initialize(VkDevice device,VkPhysicalDevice physicalDevice) {
 if(device==VK_NULL_HANDLE||physicalDevice==VK_NULL_HANDLE) return false;
 return buffer_.Initialize(device,physicalDevice,sizeof(Mat4)*kMaxBones,VulkanBufferType::UniformBuffer,
   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}
bool GPUSkinningPaletteBuffer::UploadPalette(const std::vector<Mat4>& palette) {
 if(!buffer_.IsValid()||palette.empty()||palette.size()>kMaxBones) return false;
 if(!buffer_.UploadData(palette.data(),static_cast<VkDeviceSize>(palette.size()*sizeof(Mat4)))) return false;
 boneCount_=palette.size();
 return true;
}
void GPUSkinningPaletteBuffer::Destroy(){ buffer_.Destroy(); boneCount_=0U; }
}
