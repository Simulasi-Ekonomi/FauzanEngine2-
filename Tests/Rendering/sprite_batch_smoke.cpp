#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SpriteBatch.h"

#include <cstdio>

int main() {
    using namespace NeoEngine; SoftwareRenderer renderer; RenderCamera camera; SpriteBatch batch;
    if (!renderer.Initialize(64,64) || !renderer.Clear(0xFF000000) || !camera.Initialize({RenderCameraMode::Orthographic, {}, 4.0F, 60.0F, 1.0F, 0.1F, 10.0F}) || !batch.Queue({0,0,1,4,4,0,0,0xFFFF0000}) || !batch.Queue({0,0,1,2,2,1,0,0xFF00FF00}) || !batch.Flush(renderer,camera) || renderer.PixelAt(32,32)!=0xFF00FF00 || batch.Count()!=2) return 1;
    const uint64_t hash = renderer.FrameHash(); batch.Clear(); if (batch.Count()!=0 || batch.Queue({0,0,1,0,1,0,0,0xFFFFFFFF}) || batch.LastError()!=SpriteBatchError::InvalidSize) return 1;
    std::printf("SPRITE_BATCH_SMOKE_OK sprites=2 stableLayer=1 pixel=green hash=%llu\n",static_cast<unsigned long long>(hash)); return 0;
}
