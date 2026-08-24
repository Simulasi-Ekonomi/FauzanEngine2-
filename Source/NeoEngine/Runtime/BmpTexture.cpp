#include "BmpTexture.h"

#include <limits>

namespace NeoEngine {
namespace {
bool ReadU16(std::span<const uint8_t> bytes,size_t offset,uint16_t& value){if(offset+2U>bytes.size())return false;value=static_cast<uint16_t>(bytes[offset])|static_cast<uint16_t>(bytes[offset+1U])<<8U;return true;}
bool ReadU32(std::span<const uint8_t> bytes,size_t offset,uint32_t& value){if(offset+4U>bytes.size())return false;value=static_cast<uint32_t>(bytes[offset])|static_cast<uint32_t>(bytes[offset+1U])<<8U|static_cast<uint32_t>(bytes[offset+2U])<<16U|static_cast<uint32_t>(bytes[offset+3U])<<24U;return true;}
}
bool BmpTextureDecoder::DecodeBiRgb(std::span<const uint8_t> bytes,RgbaTexture& texture,TextureDecodeError& error){
    texture={};error=TextureDecodeError::None;
    if(bytes.size()<54U||bytes[0U]!='B'||bytes[1U]!='M'){error=TextureDecodeError::UnsupportedFormat;return false;}
    uint32_t declaredBytes=0,pixelOffset=0,dibSize=0,width=0,rawHeight=0,compression=0,imageBytes=0;uint16_t planes=0,bitsPerPixel=0;
    if(!ReadU32(bytes,2U,declaredBytes)||!ReadU32(bytes,10U,pixelOffset)||!ReadU32(bytes,14U,dibSize)||!ReadU32(bytes,18U,width)||!ReadU32(bytes,22U,rawHeight)||!ReadU16(bytes,26U,planes)||!ReadU16(bytes,28U,bitsPerPixel)||!ReadU32(bytes,30U,compression)||!ReadU32(bytes,34U,imageBytes)||declaredBytes!=bytes.size()||dibSize!=40U||pixelOffset<54U){error=TextureDecodeError::InvalidHeader;return false;}
    const int64_t height=rawHeight&0x80000000U?-static_cast<int64_t>((~rawHeight)+1U):static_cast<int64_t>(rawHeight);
    if(width==0U||width>kMaxDimension||height==0||height>static_cast<int64_t>(kMaxDimension)||height<-static_cast<int64_t>(kMaxDimension)||width>kMaxPixels/static_cast<uint32_t>(height<0?-height:height)){error=TextureDecodeError::DimensionLimit;return false;}
    if(planes!=1U||compression!=0U||(bitsPerPixel!=24U&&bitsPerPixel!=32U)){error=TextureDecodeError::UnsupportedFormat;return false;}
    const uint32_t absoluteHeight=static_cast<uint32_t>(height<0?-height:height),bytesPerPixel=bitsPerPixel/8U,rowBytes=width*bytesPerPixel,rowStride=(rowBytes+3U)&~3U;
    if(rowStride<rowBytes||absoluteHeight>std::numeric_limits<size_t>::max()/rowStride){error=TextureDecodeError::DimensionLimit;return false;}
    const size_t expectedBytes=static_cast<size_t>(rowStride)*absoluteHeight;
    if(imageBytes!=0U&&imageBytes!=expectedBytes){error=TextureDecodeError::ByteCountMismatch;return false;}
    if(pixelOffset>bytes.size()||bytes.size()-pixelOffset!=expectedBytes){error=TextureDecodeError::ByteCountMismatch;return false;}
    const size_t pixelCount=static_cast<size_t>(width)*absoluteHeight;
    texture.width=static_cast<uint16_t>(width);texture.height=static_cast<uint16_t>(absoluteHeight);texture.rgba.resize(pixelCount*4U);
    for(uint32_t y=0;y<absoluteHeight;++y){const uint32_t sourceY=height>0?absoluteHeight-1U-y:y;const size_t sourceRow=static_cast<size_t>(pixelOffset)+static_cast<size_t>(sourceY)*rowStride;for(uint32_t x=0;x<width;++x){const size_t source=sourceRow+static_cast<size_t>(x)*bytesPerPixel,target=(static_cast<size_t>(y)*width+x)*4U;texture.rgba[target]=bytes[source+2U];texture.rgba[target+1U]=bytes[source+1U];texture.rgba[target+2U]=bytes[source];texture.rgba[target+3U]=bitsPerPixel==32U?bytes[source+3U]:255U;}}
    return true;
}
} // namespace NeoEngine
