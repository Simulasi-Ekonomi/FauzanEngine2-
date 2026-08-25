#include "Demos/KinematicPreflightSurfaceDemo.h"

#include "Runtime/GameplayPhysicsBody.h"
#include "Runtime/KinematicCollisionPreflight.h"
#include "Runtime/KinematicMotionController.h"
#include "Runtime/MovementAuthority.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/SpriteBatch.h"
#include "Runtime/TextureImportPipeline.h"

#include <vector>

namespace NeoEngine {
bool RunKinematicPreflightSurfaceDemo(const KinematicPreflightSurfaceDemoConfig& config, KinematicPreflightSurfaceDemoReceipt& receipt, KinematicPreflightSurfaceDemoError& error){
    receipt={};error=KinematicPreflightSurfaceDemoError::None;if(config.width<32U||config.height<32U||config.width>1024U||config.height>1024U||config.frames<3U||config.frames>600U||config.ppmPath.empty()||config.ppmPath.size()>256U){error=KinematicPreflightSurfaceDemoError::InvalidConfiguration;return false;}
    AssetRegistry registry;TextureStagingStore textures;TextureImportPipeline textureImport;TextureImportReceipt textureReceipt{};const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',120U,220U,255U};if(!textureImport.Import(registry,textures,"preflight-demo.sprite",{},ppm,TextureImportFormat::PpmP6,textureReceipt)){error=KinematicPreflightSurfaceDemoError::TextureImportFailed;return false;}
    SceneWorld world;SceneEntity actor{};if(!world.Create(actor)){error=KinematicPreflightSurfaceDemoError::WorldCreateFailed;return false;}if(!world.SetTransform(actor,{-0.75F,0.0F,3.0F,0.0F,0.0F,0.0F,1.0F,1.0F,1.0F})||!world.UpdateTransforms()){error=KinematicPreflightSurfaceDemoError::TransformFailed;return false;}
    ArchetypeManager entities;GameplayPhysicsBodyBuilder bodies;EntityID obstacle=0;if(!bodies.CreateCircleBody(entities,{GameplayPhysicsBodyType::Static,0.0F,3.0F,0.0F,0.0F,0.15F,0.0F},obstacle)){error=KinematicPreflightSurfaceDemoError::ObstacleCreateFailed;return false;}XPBDPhysicsSystem physics;physics.Step(entities,1.0F/60.0F);physics.SetEntityLayer(0U,COLLISION_LAYER_STATIC);
    KinematicMotionController motion;if(!motion.Initialize({2.0F,0.25F,false})){error=KinematicPreflightSurfaceDemoError::MotionInitializeFailed;return false;}KinematicCollisionPreflight preflight;if(!preflight.Initialize({COLLISION_LAYER_STATIC,0.0F})){error=KinematicPreflightSurfaceDemoError::PreflightInitializeFailed;return false;}
    const CpuTextureResource* texture=textures.Find("preflight-demo.sprite");SceneSpriteAdapter sprites;if(texture==nullptr||!sprites.AddStaged(actor,*texture,1.0F,1.0F,0,0,0xFFFFFFFFU)){error=KinematicPreflightSurfaceDemoError::SpriteBindFailed;return false;}
    SoftwareRenderer renderer;RenderCamera camera;SoftwareSurfacePresenter surface;if(!renderer.Initialize(config.width,config.height)){error=KinematicPreflightSurfaceDemoError::RendererInitializeFailed;return false;}if(!camera.Initialize({RenderCameraMode::Orthographic,{},5.0F,60.0F,static_cast<float>(config.width)/static_cast<float>(config.height),0.1F,10.0F})){error=KinematicPreflightSurfaceDemoError::CameraInitializeFailed;return false;}if(!surface.Initialize({config.width,config.height,config.hiddenSurface})){error=KinematicPreflightSurfaceDemoError::SurfaceInitializeFailed;return false;}
    constexpr uint32_t kClear=0xFF101420U;MovementAuthorityGate authority;uint32_t delegated=0U,blocked=0U;for(uint32_t frame=0U;frame<config.frames;++frame){const KinematicPlanarInput input=frame<2U?KinematicPlanarInput{1.0F,0.0F}:KinematicPlanarInput{};authority.BeginFrame();if(!authority.Acquire(actor,MovementAuthority::KinematicRoute)){error=KinematicPreflightSurfaceDemoError::AuthorityFailed;return false;}if(!preflight.Step(physics,world,actor,input,0.25F,motion)){if(preflight.LastError()!=KinematicCollisionPreflightError::Blocked){error=KinematicPreflightSurfaceDemoError::PreflightFailed;return false;}++blocked;}else{++delegated;}if(!world.UpdateTransforms()){error=KinematicPreflightSurfaceDemoError::TransformFailed;return false;}if(!renderer.Clear(kClear)){error=KinematicPreflightSurfaceDemoError::ClearFailed;return false;}SpriteBatch batch;if(!sprites.Queue(world,batch)){error=KinematicPreflightSurfaceDemoError::SpriteQueueFailed;return false;}if(!batch.Flush(renderer,camera)){error=KinematicPreflightSurfaceDemoError::SpriteFlushFailed;return false;}if(!surface.PumpEvents()){error=KinematicPreflightSurfaceDemoError::SurfacePumpFailed;return false;}if(surface.CloseRequested()){error=KinematicPreflightSurfaceDemoError::SurfaceCloseRequested;return false;}if(!surface.Present(renderer)){error=KinematicPreflightSurfaceDemoError::SurfacePresentFailed;return false;}}
    if(!renderer.WritePpm(config.ppmPath)){error=KinematicPreflightSurfaceDemoError::ArtifactWriteFailed;return false;}const Transform3* finalTransform=world.GetTransform(actor);if(finalTransform==nullptr){error=KinematicPreflightSurfaceDemoError::TransformFailed;return false;}uint32_t visible=0U;for(uint32_t pixel:renderer.Pixels())if(pixel!=kClear)++visible;receipt={config.frames,static_cast<uint32_t>(surface.PresentedFrameCount()),visible,delegated,blocked,renderer.FrameHash(),finalTransform->x};return true;
}
} // namespace NeoEngine
