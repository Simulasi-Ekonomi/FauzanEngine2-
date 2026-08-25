#include "Runtime/MeshRenderer.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/TextureStaging.h"

#include <cstdio>
#include <vector>

namespace {
void U16(std::vector<uint8_t>& bytes,uint16_t value){bytes.push_back(static_cast<uint8_t>(value));bytes.push_back(static_cast<uint8_t>(value>>8U));}
void U32(std::vector<uint8_t>& bytes,uint32_t value){for(uint8_t shift=0;shift<32U;shift+=8U)bytes.push_back(static_cast<uint8_t>(value>>shift));}
std::vector<uint8_t> BlueBmp(){std::vector<uint8_t> bytes{'B','M'};U32(bytes,58U);U16(bytes,0U);U16(bytes,0U);U32(bytes,54U);U32(bytes,40U);U32(bytes,1U);U32(bytes,1U);U16(bytes,1U);U16(bytes,24U);U32(bytes,0U);U32(bytes,4U);U32(bytes,0U);U32(bytes,0U);U32(bytes,0U);U32(bytes,0U);bytes.insert(bytes.end(),{255U,0U,0U,0U});return bytes;}
}
int main() {
    using namespace NeoEngine; SoftwareRenderer renderer; RenderCamera camera; MeshRenderer mesh;
    const std::vector<MeshVertex> triangle{{{-0.6F,-0.5F,2.0F},{0,0,1}},{{0.6F,-0.5F,2.0F},{0,0,1}},{{0,0.6F,2.0F},{0,0,1}}}; const std::vector<uint16_t> indices{0,1,2};
    if(!renderer.Initialize(64,64)||!renderer.Clear(0xFF000000)||!camera.Initialize({RenderCameraMode::Perspective,{},5.0F,60.0F,1.0F,0.1F,10.0F})||!mesh.Draw(triangle,indices,{}, {0xFF4080FF,0.1F,0.9F},{{0,0,1}},camera,renderer)||renderer.PixelAt(32,32)==0xFF000000||renderer.DepthAt(32,32)>=1.0F) { std::printf("MESH_RENDERER_SMOKE_FAIL stage=initial error=%u pixel=%u depth=%f\n",static_cast<unsigned>(mesh.LastError()),renderer.PixelAt(32,32),renderer.DepthAt(32,32)); return 1; }
    const std::vector<MeshVertex> farTriangle{{{-0.6F,-0.5F,4.0F},{0,0,1}},{{0.6F,-0.5F,4.0F},{0,0,1}},{{0,0.6F,4.0F},{0,0,1}}};
    if(!renderer.Clear(0xFF000000)||!mesh.Draw(farTriangle,indices,{}, {0xFF0000FF,1.0F,0.0F},{{0,0,1}},camera,renderer)||!mesh.Draw(triangle,indices,{}, {0xFFFF0000,1.0F,0.0F},{{0,0,1}},camera,renderer)||renderer.PixelAt(32,32)!=0xFFFF0000) { std::printf("MESH_RENDERER_SMOKE_FAIL stage=depth error=%u pixel=%08x\n",static_cast<unsigned>(mesh.LastError()),renderer.PixelAt(32,32)); return 1; }
    const std::vector<uint16_t> backwardIndices{0,2,1};
    if(!renderer.Clear(0xFF000000)||!mesh.Draw(triangle,indices,{},{0xFFFF0000,1.0F,0.0F,nullptr,true},{{0,0,1}},camera,renderer)||renderer.PixelAt(32,32)!=0xFFFF0000||!renderer.Clear(0xFF000000)||!mesh.Draw(triangle,backwardIndices,{},{0xFFFF0000,1.0F,0.0F,nullptr,true},{{0,0,1}},camera,renderer)||renderer.PixelAt(32,32)!=0xFF000000||!mesh.Draw(triangle,backwardIndices,{},{0xFFFF0000,1.0F,0.0F},{{0,0,1}},camera,renderer)||renderer.PixelAt(32,32)!=0xFFFF0000){std::printf("MESH_RENDERER_SMOKE_FAIL stage=culling error=%u pixel=%08x\n",static_cast<unsigned>(mesh.LastError()),renderer.PixelAt(32,32));return 1;}
    const std::vector<MeshVertex> nearTriangle{{{0.0F,0.0F,0.05F},{0,0,1}},{{0.06F,-0.06F,0.30F},{0,0,1}},{{-0.06F,0.08F,0.30F},{0,0,1}}};const std::vector<MeshVertex> behindTriangle{{{0.0F,0.0F,0.02F},{0,0,1}},{{0.02F,0.0F,0.03F},{0,0,1}},{{0.0F,0.02F,0.04F},{0,0,1}}};
    if(!renderer.Clear(0xFF000000)||!mesh.Draw(nearTriangle,indices,{},{0xFF00FF00,1.0F,0.0F},{{0,0,1}},camera,renderer)||renderer.FrameHash()==1469598103934665603ULL){std::printf("MESH_RENDERER_SMOKE_FAIL stage=near-clip error=%u\n",static_cast<unsigned>(mesh.LastError()));return 1;}const uint64_t nearHash=renderer.FrameHash();
    if(!renderer.Clear(0xFF000000)||!mesh.Draw(nearTriangle,indices,{},{0xFF00FF00,1.0F,0.0F},{{0,0,1}},camera,renderer)||renderer.FrameHash()!=nearHash||!renderer.Clear(0xFF000000)||!mesh.Draw(behindTriangle,indices,{},{0xFF00FF00,1.0F,0.0F},{{0,0,1}},camera,renderer)||renderer.PixelAt(32,32)!=0xFF000000){std::printf("MESH_RENDERER_SMOKE_FAIL stage=near-determinism error=%u\n",static_cast<unsigned>(mesh.LastError()));return 1;}
    const uint64_t hash=renderer.FrameHash();
    AssetRegistry registry; TextureStagingStore staging; const std::vector<uint8_t> ppm{'P','6','\n','2',' ','1','\n','2','5','5','\n',255,0,0,0,255,0};
    if(!registry.ImportBytes("texture-mesh-smoke",AssetKind::Texture,{},ppm)||!registry.MarkReady("texture-mesh-smoke")||!staging.StagePpm(registry,"texture-mesh-smoke")) { std::printf("MESH_RENDERER_SMOKE_FAIL stage=staging error=%u\n",static_cast<unsigned>(staging.LastError())); return 1; }
    const CpuTextureResource* texture=staging.Find("texture-mesh-smoke"); const std::vector<MeshVertex> texturedTriangle{{{-0.6F,-0.5F,2.0F},{0,0,1},0,0},{{0.6F,-0.5F,2.0F},{0,0,1},1,0},{{0,0.6F,2.0F},{0,0,1},0.5F,1}};
    if(!texture||!renderer.Clear(0xFF000000)||!mesh.Draw(texturedTriangle,indices,{},{0xFFFFFFFF,1.0F,0.0F,texture},{{0,0,1}},camera,renderer)||renderer.PixelAt(32,32)==0xFF000000) { std::printf("MESH_RENDERER_SMOKE_FAIL stage=texture error=%u pixel=%08x\n",static_cast<unsigned>(mesh.LastError()),renderer.PixelAt(32,32)); return 1; }
    const uint64_t litTextureHash=renderer.FrameHash(); if(!renderer.Clear(0xFF000000)||!mesh.Draw(texturedTriangle,indices,{},{0xFFFFFFFF,0.2F,0.0F,texture},{{0,0,1}},camera,renderer)||renderer.FrameHash()==litTextureHash) { std::printf("MESH_RENDERER_SMOKE_FAIL stage=light error=%u\n",static_cast<unsigned>(mesh.LastError())); return 1; }
    const std::vector<uint8_t> bmp=BlueBmp();
    if(!registry.ImportBytes("texture-bmp-smoke",AssetKind::Texture,{},bmp)||!registry.MarkReady("texture-bmp-smoke")||!staging.StageBmp(registry,"texture-bmp-smoke")){std::printf("MESH_RENDERER_SMOKE_FAIL stage=bmp-staging error=%u\n",static_cast<unsigned>(staging.LastError()));return 1;}
    const CpuTextureResource* bmpTexture=staging.Find("texture-bmp-smoke");
    if(!bmpTexture||!renderer.Clear(0xFF000000)||!mesh.Draw(texturedTriangle,indices,{},{0xFFFFFFFF,1.0F,0.0F,bmpTexture},{{0,0,1}},camera,renderer)||renderer.PixelAt(32,32)!=0xFF0000FFU){std::printf("MESH_RENDERER_SMOKE_FAIL stage=bmp-sample error=%u pixel=%08x\n",static_cast<unsigned>(mesh.LastError()),renderer.PixelAt(32,32));return 1;}
    const CpuTextureResource malformed{"bad",1,TextureSourceFormat::PpmP6,2,1,{0}};
    const CpuTextureResource whiteTexture{"white",1,TextureSourceFormat::PpmP6,1,1,{255U,255U,255U,255U}};
    if(!camera.Initialize({RenderCameraMode::Perspective,{},5.0F,60.0F,1.0F,0.1F,10.0F})||!renderer.Clear(0xFF000000)||!mesh.Draw(texturedTriangle,indices,{},{0xFFFF0000,1.0F,0.0F,&whiteTexture},{{0,0,1},1.0F},camera,renderer)||renderer.PixelAt(32,32)!=0xFFFF0000U||!renderer.Clear(0xFF000000)||!mesh.Draw(triangle,indices,{},{0xFFFFFFFF,0.0F,1.0F},{{0,0,1},0.5F},camera,renderer)||renderer.PixelAt(32,32)!=0xFF808080U){std::printf("MESH_RENDERER_SMOKE_FAIL stage=tint-intensity error=%u pixel=%08x\n",static_cast<unsigned>(mesh.LastError()),renderer.PixelAt(32,32));return 1;}
    const std::vector<MeshVertex> xFacingTriangle{{{2.0F,-0.5F,-0.6F},{1,0,0}},{{2.0F,-0.5F,0.6F},{1,0,0}},{{2.0F,0.6F,0.0F},{1,0,0}}};
    if(!camera.Initialize({RenderCameraMode::Perspective,{},5.0F,60.0F,1.0F,0.1F,10.0F,{1.0F,0.0F,0.0F},{0.0F,1.0F,0.0F}})||!renderer.Clear(0xFF000000)||!mesh.Draw(xFacingTriangle,indices,{},{0xFFFF8000,1.0F,0.0F},{{1,0,0}},camera,renderer)||renderer.PixelAt(32,32)==0xFF000000){std::printf("MESH_RENDERER_SMOKE_FAIL stage=oriented-camera error=%u pixel=%08x\n",static_cast<unsigned>(mesh.LastError()),renderer.PixelAt(32,32));return 1;}
    if(mesh.Draw(texturedTriangle,indices,{},{0xFFFFFFFF,0.2F,0.0F,&malformed},{{0,0,1}},camera,renderer)||mesh.LastError()!=MeshRenderError::InvalidTexture||mesh.Draw(triangle,{0,1,3},{},{0xFFFFFFFF,0.2F,0.8F},{{0,0,1}},camera,renderer)||mesh.LastError()!=MeshRenderError::InvalidIndex||mesh.Draw(triangle,indices,{},{0xFFFFFFFF,0.2F,0.8F},{{0,0,0}},camera,renderer)||mesh.LastError()!=MeshRenderError::InvalidLight||mesh.Draw(triangle,indices,{},{0xFFFFFFFF,0.2F,0.8F},{{0,0,1},9.0F},camera,renderer)||mesh.LastError()!=MeshRenderError::InvalidLight) return 1;
    std::printf("MESH_RENDERER_SMOKE_OK triangles=2 perspective=1 orientedCamera=1 texture=ppm+bmp tint=1 intensity=1 lights=2 depth=1 hash=%llu\n",static_cast<unsigned long long>(hash)); return 0;
}
