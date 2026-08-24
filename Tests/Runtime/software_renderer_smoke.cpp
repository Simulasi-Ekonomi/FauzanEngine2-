#include "Runtime/SoftwareRenderer.h"

#include <cstdio>
#include <vector>

int main(){
    using namespace NeoEngine;
    SoftwareRenderer renderer;
    if(!renderer.Initialize(64,64)||!renderer.Clear(0xFF102030)||!renderer.DrawTriangle({-.8F,-.8F,0,0xFFFF2040},{.8F,-.8F,0,0xFFFF2040},{0,.8F,0,0xFFFF2040}))return 1;
    const std::vector<uint8_t> texture{255U,0U,0U,255U,0U,255U,0U,255U,0U,0U,255U,255U,255U,255U,255U,255U};
    const TexturedRenderVertex a{-0.8F,-0.8F,0.5F,0.0F,0.0F,1.0F,1.0F},b{0.8F,-0.8F,0.5F,1.0F,0.0F,1.0F,0.25F},c{-0.8F,0.8F,0.5F,0.0F,1.0F,1.0F,1.0F};
    if(!renderer.Clear(0xFF102030)||!renderer.DrawTexturedTriangle(a,b,c,texture,4,1)){std::printf("SOFTWARE_RENDERER_SMOKE_FAIL stage=draw\n");return 1;}
    const uint32_t probe=renderer.PixelAt(28,38);
    if(probe!=0xFFFF0000U){std::printf("SOFTWARE_RENDERER_SMOKE_FAIL stage=perspective probe=%08x\n",probe);return 1;}
    if(renderer.DrawTexturedTriangle({a.x,a.y,a.z,a.u,a.v,a.light,0.0F},b,c,texture,4,1)){std::printf("SOFTWARE_RENDERER_SMOKE_FAIL stage=reciprocal-validation\n");return 1;}
    const std::vector<uint8_t> whiteTexture{255U,255U,255U,255U};
    const TexturedRenderVertex litA{a.x,a.y,a.z,0.0F,0.0F,1.0F,1.0F},litB{b.x,b.y,b.z,0.0F,0.0F,0.0F,0.25F},litC{c.x,c.y,c.z,0.0F,0.0F,1.0F,1.0F};
    if(!renderer.Clear(0xFF102030)||!renderer.DrawTexturedTriangle(litA,litB,litC,whiteTexture,1,1)){std::printf("SOFTWARE_RENDERER_SMOKE_FAIL stage=light-draw\n");return 1;}
    const uint32_t lightProbe=renderer.PixelAt(28,38)&0xFFU;
    if(lightProbe<210U||lightProbe>220U){std::printf("SOFTWARE_RENDERER_SMOKE_FAIL stage=perspective-light probe=%u\n",lightProbe);return 1;}
    if(!renderer.WritePpm("software_renderer_smoke.ppm"))return 1;
    const uint64_t hash=renderer.FrameHash();if(hash==0U)return 1;
    std::printf("SOFTWARE_RENDERER_SMOKE_OK perspectiveUv=1 hash=%llu size=%ux%u\n",static_cast<unsigned long long>(hash),renderer.Width(),renderer.Height());
}
