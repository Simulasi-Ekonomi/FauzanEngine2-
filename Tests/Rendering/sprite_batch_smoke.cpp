#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SpriteBatch.h"
#include "Runtime/TextureStaging.h"

#include <cstdio>

int main() {
    using namespace NeoEngine; SoftwareRenderer renderer; RenderCamera camera; SpriteBatch batch;
    if (!renderer.Initialize(64,64) || !renderer.Clear(0xFF000000) || !camera.Initialize({RenderCameraMode::Orthographic, {}, 4.0F, 60.0F, 1.0F, 0.1F, 10.0F}) || !batch.Queue({0,0,1,4,4,0,0,0xFFFF0000}) || !batch.Queue({0,0,1,2,2,1,0,0xFF00FF00}) || !batch.Flush(renderer,camera) || renderer.PixelAt(32,32)!=0xFF00FF00 || batch.Count()!=2) return 1;
    batch.Clear(); if (!renderer.Clear(0xFF0000FFU) || !batch.Queue({0,0,1,2,2,0,0,0x80FF0000U}) || !batch.Flush(renderer,camera) || renderer.PixelAt(28,32) != 0xFF80007FU) return 1;
    CpuTextureResource texture{"sprite.tint", 1U, TextureSourceFormat::PpmP6, 1, 1, {255U,255U,255U,255U}};
    batch.Clear(); if (!renderer.Clear(0xFF0000FFU) || !batch.Queue({0,0,1,2,2,0,0,0x80FF0000U,&texture}) || !batch.Flush(renderer,camera) || renderer.PixelAt(28,32) != 0xFF80007FU) return 1;
    const uint64_t hash = renderer.FrameHash(); batch.Clear(); if (!renderer.Clear(0xFF123456U) || !batch.Queue({10,0,1,2,2,0,0,0xFFFFFFFFU})) return 1; const uint64_t outsidePrior = renderer.FrameHash(); if (!batch.Flush(renderer,camera) || renderer.FrameHash()!=outsidePrior) return 1;
    batch.Clear(); if (!renderer.Clear(0xFF123456U) || !batch.Queue({3.5F,0,1,2,2,0,0,0xFFFFFFFFU})) return 1; const uint64_t crossingPrior = renderer.FrameHash(); if (!batch.Flush(renderer,camera) || renderer.FrameHash()==crossingPrior) return 1;
    batch.Clear(); if (!renderer.Clear(0xFF123456U) || !batch.Queue({0,0,1,3,1,0,0,0xFFFFFFFFU})) return 1; if (!batch.Flush(renderer,camera)) return 1; const uint64_t unrotatedHash=renderer.FrameHash(); batch.Clear(); if (!renderer.Clear(0xFF123456U) || !batch.Queue({0,0,1,3,1,0,0,0xFFFFFFFFU,nullptr,1.570796F}) || !batch.Flush(renderer,camera) || renderer.FrameHash()==unrotatedHash) return 1;
    RenderCamera oriented; if(!oriented.Initialize({RenderCameraMode::Perspective,{},5,90,1,0.1F,20,{1,0,0},{0,1,0}})) return 1; batch.Clear(); if(!renderer.Clear(0xFF000000U)||!batch.Queue({5,0,0,2,2,0,0,0xFFFFFFFFU,nullptr,0.0F,true})||!batch.Flush(renderer,oriented)||renderer.PixelAt(32,32)==0xFF000000U) return 1;
    batch.Clear(); if (!renderer.Clear(0xFF123456U) || !batch.Queue({0,0,1,2,2,0,0,0xFFFFFFFFU}) || !batch.Queue({0,0,1,1e-8F,1e-8F,0,1,0xFFFFFFFFU})) return 1; const uint64_t prior = renderer.FrameHash(); if (batch.Flush(renderer,camera) || batch.LastError()!=SpriteBatchError::DrawFailed || renderer.FrameHash()!=prior) return 1;
    batch.Clear(); if (batch.Count()!=0 || batch.Queue({0,0,1,0,1,0,0,0xFFFFFFFF}) || batch.LastError()!=SpriteBatchError::InvalidSize) return 1;
    std::printf("SPRITE_BATCH_SMOKE_OK sprites=2 stableLayer=1 alphaTint=1 frustumClip=1 atomicReject=1 pixel=blend hash=%llu\n",static_cast<unsigned long long>(hash)); return 0;
}
