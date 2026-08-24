#include "SoftwareRenderer.h"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace NeoEngine {
namespace {
float Edge(float ax, float ay, float bx, float by, float px, float py) { return (px - ax) * (by - ay) - (py - ay) * (bx - ax); }
bool ValidClip(float x, float y, float z) { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && z >= 0.0F && z <= 1.0F; }
}

bool SoftwareRenderer::Initialize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || width > 4096 || height > 4096 || static_cast<uint64_t>(width) * height > 16000000) return false;
    m_Width = width; m_Height = height; m_Pixels.assign(static_cast<size_t>(width) * height, 0xFF000000); m_Depth.assign(m_Pixels.size(), 1.0F); return true;
}
bool SoftwareRenderer::Clear(uint32_t rgba) { if (m_Pixels.empty() || m_Depth.size() != m_Pixels.size()) return false; std::fill(m_Pixels.begin(), m_Pixels.end(), rgba); std::fill(m_Depth.begin(), m_Depth.end(), 1.0F); return true; }

bool SoftwareRenderer::DrawTriangle(const RenderVertex& a, const RenderVertex& b, const RenderVertex& c) {
    if (m_Pixels.empty() || !ValidClip(a.x,a.y,a.z) || !ValidClip(b.x,b.y,b.z) || !ValidClip(c.x,c.y,c.z)) return false;
    const auto sx=[this](float x){return (x*0.5F+0.5F)*static_cast<float>(m_Width-1);}; const auto sy=[this](float y){return (1.0F-(y*0.5F+0.5F))*static_cast<float>(m_Height-1);};
    const float ax=sx(a.x), ay=sy(a.y), bx=sx(b.x), by=sy(b.y), cx=sx(c.x), cy=sy(c.y), area=Edge(ax,ay,bx,by,cx,cy); if (std::fabs(area)<1e-6F) return false;
    const int minX=std::max(0,static_cast<int>(std::floor(std::min({ax,bx,cx})))), maxX=std::min(static_cast<int>(m_Width)-1,static_cast<int>(std::ceil(std::max({ax,bx,cx})))), minY=std::max(0,static_cast<int>(std::floor(std::min({ay,by,cy})))), maxY=std::min(static_cast<int>(m_Height)-1,static_cast<int>(std::ceil(std::max({ay,by,cy}))));
    for (int y=minY;y<=maxY;++y) for (int x=minX;x<=maxX;++x) { const float wA=Edge(bx,by,cx,cy,static_cast<float>(x),static_cast<float>(y))/area, wB=Edge(cx,cy,ax,ay,static_cast<float>(x),static_cast<float>(y))/area, wC=1.0F-wA-wB; if (wA>=0.0F&&wB>=0.0F&&wC>=0.0F) { const size_t index=static_cast<size_t>(y)*m_Width+x; const float depth=wA*a.z+wB*b.z+wC*c.z; if(depth<=m_Depth[index]){m_Depth[index]=depth;m_Pixels[index]=a.rgba;} } }
    return true;
}

bool SoftwareRenderer::DrawTexturedTriangle(const TexturedRenderVertex& a, const TexturedRenderVertex& b, const TexturedRenderVertex& c, const std::vector<uint8_t>& rgba, uint16_t textureWidth, uint16_t textureHeight) {
    const auto valid=[](const TexturedRenderVertex& v){return ValidClip(v.x,v.y,v.z)&&std::isfinite(v.u)&&std::isfinite(v.v)&&std::isfinite(v.light)&&std::isfinite(v.reciprocalDepth)&&v.u>=0.0F&&v.u<=1.0F&&v.v>=0.0F&&v.v<=1.0F&&v.light>=0.0F&&v.light<=1.0F&&v.reciprocalDepth>0.0F;};
    if(m_Pixels.empty() || textureWidth==0 || textureHeight==0 || textureWidth>4096 || textureHeight>4096 || rgba.size()!=static_cast<size_t>(textureWidth)*textureHeight*4U || !valid(a) || !valid(b) || !valid(c)) return false;
    const auto sx=[this](float x){return (x*0.5F+0.5F)*static_cast<float>(m_Width-1);}; const auto sy=[this](float y){return (1.0F-(y*0.5F+0.5F))*static_cast<float>(m_Height-1);};
    const float ax=sx(a.x), ay=sy(a.y), bx=sx(b.x), by=sy(b.y), cx=sx(c.x), cy=sy(c.y), area=Edge(ax,ay,bx,by,cx,cy); if(std::fabs(area)<1e-6F) return false;
    const int minX=std::max(0,static_cast<int>(std::floor(std::min({ax,bx,cx})))), maxX=std::min(static_cast<int>(m_Width)-1,static_cast<int>(std::ceil(std::max({ax,bx,cx})))), minY=std::max(0,static_cast<int>(std::floor(std::min({ay,by,cy})))), maxY=std::min(static_cast<int>(m_Height)-1,static_cast<int>(std::ceil(std::max({ay,by,cy}))));
    for (int y=minY;y<=maxY;++y) for (int x=minX;x<=maxX;++x) { const float wA=Edge(bx,by,cx,cy,static_cast<float>(x),static_cast<float>(y))/area, wB=Edge(cx,cy,ax,ay,static_cast<float>(x),static_cast<float>(y))/area, wC=1.0F-wA-wB; if(wA>=0.0F&&wB>=0.0F&&wC>=0.0F){const size_t index=static_cast<size_t>(y)*m_Width+x;const float depth=wA*a.z+wB*b.z+wC*c.z;if(depth<=m_Depth[index]){const float reciprocalDepth=wA*a.reciprocalDepth+wB*b.reciprocalDepth+wC*c.reciprocalDepth;if(!std::isfinite(reciprocalDepth)||reciprocalDepth<=0.0F) return false;const float u=(wA*a.u*a.reciprocalDepth+wB*b.u*b.reciprocalDepth+wC*c.u*c.reciprocalDepth)/reciprocalDepth,v=(wA*a.v*a.reciprocalDepth+wB*b.v*b.reciprocalDepth+wC*c.v*c.reciprocalDepth)/reciprocalDepth,light=(wA*a.light*a.reciprocalDepth+wB*b.light*b.reciprocalDepth+wC*c.light*c.reciprocalDepth)/reciprocalDepth;if(!std::isfinite(light)||light<0.0F||light>1.0F)return false;const uint16_t tx=static_cast<uint16_t>(std::lround(u*static_cast<float>(textureWidth-1U))),ty=static_cast<uint16_t>(std::lround(v*static_cast<float>(textureHeight-1U)));const size_t texel=(static_cast<size_t>(ty)*textureWidth+tx)*4U;const auto channel=[&](size_t offset){return static_cast<uint32_t>(std::clamp(std::lround(static_cast<float>(rgba[texel+offset])*light),0L,255L));};m_Depth[index]=depth;m_Pixels[index]=0xFF000000U|(channel(0)<<16)|(channel(1)<<8)|channel(2);}}}
    return true;
}

bool SoftwareRenderer::WritePpm(const std::string& path) const { if(m_Pixels.empty()) return false; std::ofstream file(path,std::ios::binary); if(!file) return false; file<<"P6\n"<<m_Width<<' '<<m_Height<<"\n255\n"; for(uint32_t color:m_Pixels){const char rgb[3]={static_cast<char>((color>>16)&255U),static_cast<char>((color>>8)&255U),static_cast<char>(color&255U)};file.write(rgb,3);} return static_cast<bool>(file); }
uint64_t SoftwareRenderer::FrameHash() const { uint64_t hash=1469598103934665603ULL; for(uint32_t color:m_Pixels){hash^=color;hash*=1099511628211ULL;}return hash; }
} // namespace NeoEngine
