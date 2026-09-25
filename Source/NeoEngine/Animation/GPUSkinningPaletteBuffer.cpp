#include "GPUSkinningPaletteBuffer.h"
#include <array>
#include <cmath>
#include <limits>
namespace NeoEngine {
namespace {
constexpr uint32_t kPaletteSlotCount = GPUSkinningPaletteBuffer::kFramesInFlight *
                                       GPUSkinningPaletteBuffer::kMaxPalettesPerFrame;
void MakeIdentityPalette(std::array<Mat4, GPUSkinningPaletteBuffer::kMaxBones>& palette) {
 for (Mat4& matrix : palette) {
  for (float& value : matrix.m) value = 0.0F;
  matrix.m[0] = matrix.m[5] = matrix.m[10] = matrix.m[15] = 1.0F;
 }
}
}
bool GPUSkinningPaletteBuffer::Initialize(VkDevice device,VkPhysicalDevice physicalDevice) {
 if(device==VK_NULL_HANDLE||physicalDevice==VK_NULL_HANDLE||kMaxBones==0U) return false;
 VkPhysicalDeviceProperties properties{};
 vkGetPhysicalDeviceProperties(physicalDevice,&properties);
 const VkDeviceSize alignment=properties.limits.minUniformBufferOffsetAlignment==0U?1U:properties.limits.minUniformBufferOffsetAlignment;
 if(kPaletteByteSize>std::numeric_limits<VkDeviceSize>::max()-(alignment-1U)) return false;
 paletteStride_=((kPaletteByteSize+alignment-1U)/alignment)*alignment;
 if(paletteStride_>std::numeric_limits<VkDeviceSize>::max()/kPaletteSlotCount) return false;
 const VkDeviceSize totalBytes=paletteStride_*kPaletteSlotCount;
 if(totalBytes>std::numeric_limits<uint32_t>::max()) return false;
 if(!buffer_.Initialize(device,physicalDevice,totalBytes,VulkanBufferType::UniformBuffer,
   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) return false;
 std::array<Mat4,kMaxBones> identity{};
 MakeIdentityPalette(identity);
 for(uint32_t frame=0U;frame<kFramesInFlight;++frame){
  const VkDeviceSize offset=static_cast<VkDeviceSize>(frame)*kMaxPalettesPerFrame*paletteStride_;
  if(!buffer_.UploadDataAtOffset(identity.data(),kPaletteByteSize,offset)){Destroy();return false;}
 }
 frameIndex_=0U;nextPaletteIndex_=1U;dynamicOffset_=0U;boneCount_=0U;
 return true;
}
bool GPUSkinningPaletteBuffer::BeginFrame(uint32_t frameIndex) noexcept {
 if(!buffer_.IsValid()||frameIndex>=kFramesInFlight) return false;
 frameIndex_=frameIndex;nextPaletteIndex_=1U;boneCount_=0U;
 const VkDeviceSize offset=static_cast<VkDeviceSize>(frameIndex_)*kMaxPalettesPerFrame*paletteStride_;
 if(offset>std::numeric_limits<uint32_t>::max()) return false;
 dynamicOffset_=static_cast<uint32_t>(offset);
 return true;
}
bool GPUSkinningPaletteBuffer::UploadPalette(const std::vector<Mat4>& palette) {
 if(!buffer_.IsValid()||palette.empty()||palette.size()>kMaxBones||nextPaletteIndex_>=kMaxPalettesPerFrame) return false;
 for(const Mat4& matrix:palette)for(float value:matrix.m)if(!std::isfinite(value))return false;
 std::array<Mat4,kMaxBones> padded{};
 MakeIdentityPalette(padded);
 for(size_t i=0U;i<palette.size();++i)padded[i]=palette[i];
 const VkDeviceSize frameBase=static_cast<VkDeviceSize>(frameIndex_)*kMaxPalettesPerFrame*paletteStride_;
 const VkDeviceSize offset=frameBase+static_cast<VkDeviceSize>(nextPaletteIndex_)*paletteStride_;
 if(offset>std::numeric_limits<uint32_t>::max()||!buffer_.UploadDataAtOffset(padded.data(),kPaletteByteSize,offset))return false;
 dynamicOffset_=static_cast<uint32_t>(offset);++nextPaletteIndex_;boneCount_=palette.size();
 return true;
}
void GPUSkinningPaletteBuffer::UseDefaultPalette() noexcept {
 const VkDeviceSize offset=static_cast<VkDeviceSize>(frameIndex_)*kMaxPalettesPerFrame*paletteStride_;
 if(offset<=std::numeric_limits<uint32_t>::max())dynamicOffset_=static_cast<uint32_t>(offset);
 boneCount_=0U;
}
void GPUSkinningPaletteBuffer::Destroy(){buffer_.Destroy();boneCount_=0U;paletteStride_=0U;frameIndex_=0U;nextPaletteIndex_=1U;dynamicOffset_=0U;}
}
