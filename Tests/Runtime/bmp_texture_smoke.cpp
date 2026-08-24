#include "Runtime/BmpTexture.h"

#include <cstdio>
#include <vector>

namespace {
void U16(std::vector<uint8_t>& bytes,uint16_t value){bytes.push_back(static_cast<uint8_t>(value));bytes.push_back(static_cast<uint8_t>(value>>8U));}
void U32(std::vector<uint8_t>& bytes,uint32_t value){for(uint8_t shift=0;shift<32U;shift+=8U)bytes.push_back(static_cast<uint8_t>(value>>shift));}
std::vector<uint8_t> Bmp24(){std::vector<uint8_t> bytes{'B','M'};U32(bytes,62U);U16(bytes,0U);U16(bytes,0U);U32(bytes,54U);U32(bytes,40U);U32(bytes,2U);U32(bytes,1U);U16(bytes,1U);U16(bytes,24U);U32(bytes,0U);U32(bytes,8U);U32(bytes,0U);U32(bytes,0U);U32(bytes,0U);U32(bytes,0U);bytes.insert(bytes.end(),{0U,0U,255U,0U,255U,0U,0U,0U});return bytes;}
std::vector<uint8_t> Bmp32TopDown(){std::vector<uint8_t> bytes{'B','M'};U32(bytes,58U);U16(bytes,0U);U16(bytes,0U);U32(bytes,54U);U32(bytes,40U);U32(bytes,1U);U32(bytes,0xFFFFFFFFU);U16(bytes,1U);U16(bytes,32U);U32(bytes,0U);U32(bytes,4U);U32(bytes,0U);U32(bytes,0U);U32(bytes,0U);U32(bytes,0U);bytes.insert(bytes.end(),{1U,2U,3U,4U});return bytes;}
}
int main(){using namespace NeoEngine;const std::vector<uint8_t> valid=Bmp24();RgbaTexture texture;TextureDecodeError error=TextureDecodeError::None;if(!BmpTextureDecoder::DecodeBiRgb(valid,texture,error)||texture.width!=2U||texture.height!=1U||texture.rgba.size()!=8U||texture.rgba[0U]!=255U||texture.rgba[1U]!=0U||texture.rgba[4U]!=0U||texture.rgba[5U]!=255U||texture.rgba[7U]!=255U)return 1;const std::vector<uint8_t> topDown=Bmp32TopDown();if(!BmpTextureDecoder::DecodeBiRgb(topDown,texture,error)||texture.width!=1U||texture.height!=1U||texture.rgba.size()!=4U||texture.rgba[0U]!=3U||texture.rgba[1U]!=2U||texture.rgba[2U]!=1U||texture.rgba[3U]!=4U)return 1;std::vector<uint8_t> truncated=valid;truncated.pop_back();if(BmpTextureDecoder::DecodeBiRgb(truncated,texture,error)||error!=TextureDecodeError::InvalidHeader)return 1;std::vector<uint8_t> unsupported=valid;unsupported[28U]=16U;unsupported[29U]=0U;if(BmpTextureDecoder::DecodeBiRgb(unsupported,texture,error)||error!=TextureDecodeError::UnsupportedFormat)return 1;std::vector<uint8_t> wrongMagic=valid;wrongMagic[0U]='P';if(BmpTextureDecoder::DecodeBiRgb(wrongMagic,texture,error)||error!=TextureDecodeError::UnsupportedFormat)return 1;std::printf("BMP_TEXTURE_SMOKE_OK rgb24=1 rgba32=1 topDown=1 malformed=1\n");}
