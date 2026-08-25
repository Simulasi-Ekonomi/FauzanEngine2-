#include "Demos/HybridSurfaceDemo.h"

#include "Runtime/MeshRenderer.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/SpriteBatch.h"
#include "Runtime/TextureStaging.h"

#include <vector>

namespace NeoEngine {
bool RunHybridSurfaceDemo(const HybridSurfaceDemoConfig& config, HybridSurfaceDemoReceipt& receipt, HybridSurfaceDemoError& error) {
    receipt = {}; error = HybridSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames == 0U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = HybridSurfaceDemoError::InvalidConfiguration; return false; }
    SoftwareRenderer renderer; if (!renderer.Initialize(config.width, config.height)) { error = HybridSurfaceDemoError::RendererInitializeFailed; return false; }
    RenderCamera camera; if (!camera.Initialize({RenderCameraMode::Perspective, {}, 5.0F, 60.0F, static_cast<float>(config.width) / static_cast<float>(config.height), 0.1F, 20.0F})) { error = HybridSurfaceDemoError::CameraInitializeFailed; return false; }
    SoftwareSurfacePresenter surface; if (!surface.Initialize({config.width, config.height, config.hiddenSurface})) { error = HybridSurfaceDemoError::SurfaceInitializeFailed; return false; }
    const CpuTextureResource meshTexture{"hybrid-mesh-checker", 1U, TextureSourceFormat::PpmP6, 2U, 2U, {255U, 90U, 70U, 255U, 80U, 190U, 90U, 255U, 80U, 120U, 255U, 255U, 245U, 220U, 100U, 255U}};
    const std::vector<MeshVertex> meshVertices{{{-1.5F,-1.0F,4.5F},{0,0,-1},0,1},{{1.5F,-1.0F,4.5F},{0,0,-1},1,1},{{0,1.0F,4.5F},{0,0,-1},0.5F,0}};
    const std::vector<uint16_t> meshIndices{0U,1U,2U}; const MeshMaterial material{0xFFE0F0FFU,0.2F,0.8F,&meshTexture,false}; const DirectionalLight light{{0,0,-1},1.0F}; constexpr uint32_t kClear=0xFF0B1020U;
    MeshRenderer mesh;
    for (uint32_t frame = 0; frame < config.frames; ++frame) {
        if (!renderer.Clear(kClear)) { error = HybridSurfaceDemoError::ClearFailed; return false; }
        if (!mesh.Draw(meshVertices, meshIndices, {{0,0,0},1.0F,{0,static_cast<float>(frame)*0.06F,0}}, material, light, camera, renderer)) { error = HybridSurfaceDemoError::MeshDrawFailed; return false; }
        SpriteBatch sprites; const SpriteDraw billboard{0.0F,0.15F,2.8F,0.85F,1.25F,0,0,0xB0B8F4FFU,nullptr,static_cast<float>(frame)*0.10F,true};
        if (!sprites.Queue(billboard) || !sprites.Flush(renderer,camera)) { error = HybridSurfaceDemoError::SpriteDrawFailed; return false; }
        if (!surface.PumpEvents()) { error = HybridSurfaceDemoError::SurfacePumpFailed; return false; }
        if (surface.CloseRequested()) { error = HybridSurfaceDemoError::SurfaceCloseRequested; return false; }
        if (!surface.Present(renderer)) { error = HybridSurfaceDemoError::SurfacePresentFailed; return false; }
    }
    if (!renderer.WritePpm(config.ppmPath)) { error = HybridSurfaceDemoError::ArtifactWriteFailed; return false; }
    uint32_t visible = 0; for (uint32_t pixel : renderer.Pixels()) if (pixel != kClear) ++visible;
    receipt = {config.frames, static_cast<uint32_t>(surface.PresentedFrameCount()), visible, renderer.FrameHash()}; return true;
}
} // namespace NeoEngine
