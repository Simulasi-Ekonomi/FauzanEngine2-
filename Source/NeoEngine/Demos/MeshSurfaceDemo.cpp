#include "Demos/MeshSurfaceDemo.h"

#include "Runtime/MeshRenderer.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/TextureStaging.h"

#include <vector>

namespace NeoEngine {
bool RunMeshSurfaceDemo(const MeshSurfaceDemoConfig& config, MeshSurfaceDemoReceipt& receipt, MeshSurfaceDemoError& error) {
    receipt = {};
    error = MeshSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames == 0U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = MeshSurfaceDemoError::InvalidConfiguration; return false; }
    SoftwareRenderer renderer;
    if (!renderer.Initialize(config.width, config.height)) { error = MeshSurfaceDemoError::RendererInitializeFailed; return false; }
    RenderCamera camera;
    if (!camera.Initialize({RenderCameraMode::Perspective, {0.0F, 0.0F, 0.0F}, 5.0F, 60.0F, static_cast<float>(config.width) / static_cast<float>(config.height), 0.1F, 20.0F})) { error = MeshSurfaceDemoError::CameraInitializeFailed; return false; }
    SoftwareSurfacePresenter surface;
    if (!surface.Initialize({config.width, config.height, config.hiddenSurface})) { error = MeshSurfaceDemoError::SurfaceInitializeFailed; return false; }
    const CpuTextureResource checker{"mesh-surface-demo-checker", 1U, TextureSourceFormat::PpmP6, 2, 2, {255U, 72U, 72U, 255U, 72U, 255U, 96U, 255U, 72U, 112U, 255U, 255U, 255U, 238U, 92U, 255U}};
    const std::vector<MeshVertex> vertices{
        {{-0.9F, -0.8F, 3.0F}, {0.0F, 0.0F, -1.0F}, 0.0F, 1.0F},
        {{0.9F, -0.8F, 3.0F}, {0.0F, 0.0F, -1.0F}, 1.0F, 1.0F},
        {{0.0F, 0.9F, 3.0F}, {0.0F, 0.0F, -1.0F}, 0.5F, 0.0F},
        {{0.0F, 0.0F, 4.2F}, {0.0F, 0.0F, -1.0F}, 0.5F, 0.5F},
    };
    const std::vector<uint16_t> indices{0U, 1U, 2U, 0U, 3U, 1U, 1U, 3U, 2U, 2U, 3U, 0U};
    const MeshMaterial material{0xFFE6F4FFU, 0.15F, 0.85F, &checker, false};
    const DirectionalLight light{{0.0F, 0.0F, -1.0F}, 1.0F};
    constexpr uint32_t kClear = 0xFF101420U;
    MeshRenderer mesh;
    for (uint32_t frame = 0; frame < config.frames; ++frame) {
        if (!renderer.Clear(kClear)) { error = MeshSurfaceDemoError::ClearFailed; return false; }
        if (!mesh.Draw(vertices, indices, {{0.0F, 0.0F, 0.0F}, 1.0F, {0.0F, static_cast<float>(frame) * 0.08F, 0.0F}}, material, light, camera, renderer)) { error = MeshSurfaceDemoError::MeshDrawFailed; return false; }
        if (!surface.PumpEvents()) { error = MeshSurfaceDemoError::SurfacePumpFailed; return false; }
        if (surface.CloseRequested()) { error = MeshSurfaceDemoError::SurfaceCloseRequested; return false; }
        if (!surface.Present(renderer)) { error = MeshSurfaceDemoError::SurfacePresentFailed; return false; }
    }
    if (!renderer.WritePpm(config.ppmPath)) { error = MeshSurfaceDemoError::ArtifactWriteFailed; return false; }
    uint32_t visiblePixels = 0;
    for (uint32_t pixel : renderer.Pixels()) if (pixel != kClear) ++visiblePixels;
    receipt = {config.frames, static_cast<uint32_t>(surface.PresentedFrameCount()), visiblePixels, renderer.FrameHash()};
    return true;
}
} // namespace NeoEngine
