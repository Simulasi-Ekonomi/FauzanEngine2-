#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace NeoEngine { struct RenderVertex { float x,y,z; uint32_t rgba; }; struct TexturedRenderVertex { float x,y,z,u,v,light,reciprocalDepth; }; class SoftwareRenderer { public: bool Initialize(uint32_t width,uint32_t height); bool Clear(uint32_t rgba); bool DrawTriangle(const RenderVertex&a,const RenderVertex&b,const RenderVertex&c); bool DrawTexturedTriangle(const TexturedRenderVertex&a,const TexturedRenderVertex&b,const TexturedRenderVertex&c,const std::vector<uint8_t>&rgba,uint16_t textureWidth,uint16_t textureHeight); bool WritePpm(const std::string&path)const; uint64_t FrameHash()const; uint32_t Width()const{return m_Width;} uint32_t Height()const{return m_Height;} std::span<const uint32_t> Pixels() const { return m_Pixels; } uint32_t PixelAt(uint32_t x,uint32_t y)const{return x<m_Width&&y<m_Height?m_Pixels[static_cast<size_t>(y)*m_Width+x]:0;} float DepthAt(uint32_t x,uint32_t y)const{return x<m_Width&&y<m_Height?m_Depth[static_cast<size_t>(y)*m_Width+x]:1.0F;} private:uint32_t m_Width=0,m_Height=0;std::vector<uint32_t>m_Pixels;std::vector<float>m_Depth;}; }
