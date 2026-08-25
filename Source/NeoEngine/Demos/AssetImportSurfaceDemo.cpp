#include "Demos/AssetImportSurfaceDemo.h"

#include "Runtime/MeshImportPipeline.h"
#include "Runtime/MeshRenderer.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/TextureImportPipeline.h"

#include <string>
#include <vector>

namespace NeoEngine {
bool RunAssetImportSurfaceDemo(const AssetImportSurfaceDemoConfig& config, AssetImportSurfaceDemoReceipt& receipt, AssetImportSurfaceDemoError& error) {
    receipt = {}; error = AssetImportSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames == 0U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = AssetImportSurfaceDemoError::InvalidConfiguration; return false; }
    AssetRegistry registry; TextureStagingStore textures; MeshStagingStore meshes; TextureImportPipeline textureImport; MeshImportPipeline meshImport; TextureImportReceipt textureReceipt{}; MeshImportReceipt meshReceipt{};
    const std::vector<uint8_t> ppm{'P','6','\n','2',' ','2','\n','2','5','5','\n',255U,72U,72U,72U,255U,96U,72U,112U,255U,255U,238U,92U};
    const std::string obj = "v -0.9 -0.8 3\nv 0.9 -0.8 3\nv 0.9 0.9 3\nv -0.9 0.9 3\nvt 0 1\nvt 1 1\nvt 1 0\nvt 0 0\nvn 0 0 -1\nf 1/1/1 2/2/1 3/3/1 4/4/1\n";
    if (!textureImport.Import(registry, textures, "asset-demo.texture", {}, ppm, TextureImportFormat::PpmP6, textureReceipt)) { error = AssetImportSurfaceDemoError::TextureImportFailed; return false; }
    if (!meshImport.ImportObj(registry, meshes, "asset-demo.mesh", {}, std::vector<uint8_t>(obj.begin(), obj.end()), {}, meshReceipt)) { error = AssetImportSurfaceDemoError::MeshImportFailed; return false; }
    const CpuTextureResource* texture = textures.Find("asset-demo.texture"); const CpuMeshResource* meshResource = meshes.Find("asset-demo.mesh"); if (texture == nullptr || meshResource == nullptr) { error = AssetImportSurfaceDemoError::MeshImportFailed; return false; }
    SoftwareRenderer renderer; if (!renderer.Initialize(config.width, config.height)) { error = AssetImportSurfaceDemoError::RendererInitializeFailed; return false; }
    RenderCamera camera; if (!camera.Initialize({RenderCameraMode::Perspective, {0.0F, 0.0F, 0.0F}, 5.0F, 60.0F, static_cast<float>(config.width) / static_cast<float>(config.height), 0.1F, 20.0F})) { error = AssetImportSurfaceDemoError::CameraInitializeFailed; return false; }
    SoftwareSurfacePresenter surface; if (!surface.Initialize({config.width, config.height, config.hiddenSurface})) { error = AssetImportSurfaceDemoError::SurfaceInitializeFailed; return false; }
    constexpr uint32_t kClear = 0xFF101420U; const MeshMaterial material{0xFFE6F4FFU, 0.15F, 0.85F, texture, false}; const DirectionalLight light{{0.0F, 0.0F, -1.0F}, 1.0F}; MeshRenderer mesh;
    for (uint32_t frame = 0; frame < config.frames; ++frame) { if (!renderer.Clear(kClear)) { error = AssetImportSurfaceDemoError::ClearFailed; return false; } if (!mesh.Draw(meshResource->vertices, meshResource->indices, {{0.0F, 0.0F, 0.0F}, 1.0F, {0.0F, static_cast<float>(frame) * 0.08F, 0.0F}}, material, light, camera, renderer)) { error = AssetImportSurfaceDemoError::MeshDrawFailed; return false; } if (!surface.PumpEvents()) { error = AssetImportSurfaceDemoError::SurfacePumpFailed; return false; } if (surface.CloseRequested()) { error = AssetImportSurfaceDemoError::SurfaceCloseRequested; return false; } if (!surface.Present(renderer)) { error = AssetImportSurfaceDemoError::SurfacePresentFailed; return false; } }
    if (!renderer.WritePpm(config.ppmPath)) { error = AssetImportSurfaceDemoError::ArtifactWriteFailed; return false; }
    uint32_t visiblePixels = 0; for (const uint32_t pixel : renderer.Pixels()) if (pixel != kClear) ++visiblePixels;
    receipt = {config.frames, static_cast<uint32_t>(surface.PresentedFrameCount()), visiblePixels, renderer.FrameHash(), meshReceipt.contentHash, textureReceipt.contentHash}; return true;
}
} // namespace NeoEngine
