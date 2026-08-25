#include "Demos/MaterialImportSurfaceDemo.h"

#include "Runtime/MaterialImportPipeline.h"
#include "Runtime/MeshImportPipeline.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SceneMeshAdapter.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/TextureImportPipeline.h"

#include <string>
#include <vector>

namespace NeoEngine {
bool RunMaterialImportSurfaceDemo(const MaterialImportSurfaceDemoConfig& config, MaterialImportSurfaceDemoReceipt& receipt, MaterialImportSurfaceDemoError& error) {
    receipt = {}; error = MaterialImportSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames == 0U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = MaterialImportSurfaceDemoError::InvalidConfiguration; return false; }
    AssetRegistry registry; TextureStagingStore textures; MeshStagingStore meshes; MaterialStagingStore materials; TextureImportPipeline textureImport; MeshImportPipeline meshImport; MaterialImportPipeline materialImport; TextureImportReceipt textureReceipt{}; MeshImportReceipt meshReceipt{}; MaterialImportReceipt materialReceipt{};
    const std::vector<uint8_t> ppm{'P','6','\n','2',' ','2','\n','2','5','5','\n',255U,72U,72U,72U,255U,96U,72U,112U,255U,255U,238U,92U};
    const std::string obj = "v -0.9 -0.8 3\nv 0.9 -0.8 3\nv 0.9 0.9 3\nv -0.9 0.9 3\nvt 0 1\nvt 1 1\nvt 1 0\nvt 0 0\nvn 0 0 -1\nf 1/1/1 2/2/1 3/3/1 4/4/1\n";
    const std::vector<uint8_t> mtl{'n','e','w','m','t','l',' ','f','a','r','m','\n','K','d',' ','0','.','4',' ','0','.','8',' ','0','.','6','\n','d',' ','1','.','0','\n'};
    if (!textureImport.Import(registry, textures, "material-surface.texture", {}, ppm, TextureImportFormat::PpmP6, textureReceipt)) { error = MaterialImportSurfaceDemoError::TextureImportFailed; return false; }
    if (!meshImport.ImportObj(registry, meshes, "material-surface.mesh", {}, std::vector<uint8_t>(obj.begin(), obj.end()), {}, meshReceipt)) { error = MaterialImportSurfaceDemoError::MeshImportFailed; return false; }
    if (!materialImport.ImportMtl(registry, materials, "material-surface.material", {}, mtl, "farm", materialReceipt)) { error = MaterialImportSurfaceDemoError::MaterialImportFailed; return false; }
    const CpuTextureResource* texture= textures.Find("material-surface.texture"); const CpuMeshResource* mesh=meshes.Find("material-surface.mesh"); const CpuMaterialResource* material=materials.Find("material-surface.material","farm");
    if (texture==nullptr || mesh==nullptr || material==nullptr) { error = MaterialImportSurfaceDemoError::SceneBindFailed; return false; }
    SceneWorld world; SceneEntity entity{}; if (!world.Create(entity)) { error = MaterialImportSurfaceDemoError::WorldCreateFailed; return false; } if (!world.SetTransform(entity,{}) || !world.UpdateTransforms()) { error = MaterialImportSurfaceDemoError::TransformFailed; return false; }
    SceneMeshAdapter sceneMeshes; if (!sceneMeshes.AddStaged(entity,*mesh,*material,texture)) { error = MaterialImportSurfaceDemoError::SceneBindFailed; return false; }
    SoftwareRenderer renderer; if (!renderer.Initialize(config.width,config.height)) { error = MaterialImportSurfaceDemoError::RendererInitializeFailed; return false; }
    RenderCamera camera; if (!camera.Initialize({RenderCameraMode::Perspective,{},5.0F,60.0F,static_cast<float>(config.width)/static_cast<float>(config.height),0.1F,20.0F})) { error = MaterialImportSurfaceDemoError::CameraInitializeFailed; return false; }
    SoftwareSurfacePresenter surface; if (!surface.Initialize({config.width,config.height,config.hiddenSurface})) { error = MaterialImportSurfaceDemoError::SurfaceInitializeFailed; return false; }
    constexpr uint32_t kClear=0xFF101420U; const DirectionalLight light{{0.0F,0.0F,-1.0F},1.0F};
    for(uint32_t frame=0;frame<config.frames;++frame) { if(!renderer.Clear(kClear)) { error=MaterialImportSurfaceDemoError::ClearFailed; return false; } if(!sceneMeshes.Draw(world,camera,renderer,light)) { error=MaterialImportSurfaceDemoError::SceneDrawFailed; return false; } if(!surface.PumpEvents()) { error=MaterialImportSurfaceDemoError::SurfacePumpFailed; return false; } if(surface.CloseRequested()) { error=MaterialImportSurfaceDemoError::SurfaceCloseRequested; return false; } if(!surface.Present(renderer)) { error=MaterialImportSurfaceDemoError::SurfacePresentFailed; return false; } }
    if(!renderer.WritePpm(config.ppmPath)) { error=MaterialImportSurfaceDemoError::ArtifactWriteFailed; return false; }
    uint32_t visible=0; for(uint32_t pixel:renderer.Pixels()) if(pixel!=kClear)++visible;
    receipt={config.frames,static_cast<uint32_t>(surface.PresentedFrameCount()),visible,renderer.FrameHash(),textureReceipt.contentHash,meshReceipt.contentHash,materialReceipt.contentHash,materialReceipt.rgba}; return true;
}
} // namespace NeoEngine
