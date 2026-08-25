#include "Demos/TriggerRuntimeSurfaceDemo.h"

#include "Runtime/GameplayPhysicsBody.h"
#include "Runtime/GameplayTriggerTracker.h"
#include "Runtime/KinematicMotionController.h"
#include "Runtime/MovementAuthority.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/ScenePhysicsPoseSync.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/SpriteBatch.h"
#include "Runtime/TextureImportPipeline.h"

#include <vector>

namespace NeoEngine {
bool RunTriggerRuntimeSurfaceDemo(const TriggerRuntimeSurfaceDemoConfig& config, TriggerRuntimeSurfaceDemoReceipt& receipt, TriggerRuntimeSurfaceDemoError& error) {
    receipt = {}; error = TriggerRuntimeSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames < 4U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = TriggerRuntimeSurfaceDemoError::InvalidConfiguration; return false; }
    AssetRegistry registry; TextureStagingStore textures; TextureImportPipeline textureImport; TextureImportReceipt textureReceipt{}; const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',250U,196U,68U};
    if (!textureImport.Import(registry,textures,"trigger-demo.sprite",{},ppm,TextureImportFormat::PpmP6,textureReceipt)) { error = TriggerRuntimeSurfaceDemoError::TextureImportFailed; return false; }
    SceneWorld world; SceneEntity actor{}; if (!world.Create(actor)) { error = TriggerRuntimeSurfaceDemoError::WorldCreateFailed; return false; } if (!world.SetTransform(actor,{-0.75F,0.0F,3.0F,0.0F,0.0F,0.0F,1.0F,1.0F,1.0F}) || !world.UpdateTransforms()) { error = TriggerRuntimeSurfaceDemoError::TransformFailed; return false; }
    ArchetypeManager entities; GameplayPhysicsBodyBuilder bodies; EntityID actorBody=0; if (!bodies.CreateCircleBody(entities,{GameplayPhysicsBodyType::Static,0.0F,0.0F,0.0F,0.0F,0.12F,0.0F},actorBody)) { error = TriggerRuntimeSurfaceDemoError::BodyCreateFailed; return false; }
    ScenePhysicsPoseSync poseSync; if (!poseSync.Bind(actor,actorBody)) { error = TriggerRuntimeSurfaceDemoError::PoseBindFailed; return false; } XPBDPhysicsSystem physics; physics.Step(entities,1.0F/60.0F); physics.SetEntityLayer(0U,COLLISION_LAYER_STATIC);
    KinematicMotionController motion; if (!motion.Initialize({2.0F,0.25F,false})) { error = TriggerRuntimeSurfaceDemoError::MotionInitializeFailed; return false; } GameplayTriggerTracker trigger; if (!trigger.Initialize({0.0F,3.0F,0.30F,COLLISION_LAYER_STATIC})) { error = TriggerRuntimeSurfaceDemoError::TriggerInitializeFailed; return false; }
    const CpuTextureResource* texture = textures.Find("trigger-demo.sprite"); SceneSpriteAdapter sprites; if (texture == nullptr || !sprites.AddStaged(actor,*texture,1.0F,1.0F,0,0,0xFFFFFFFFU)) { error = TriggerRuntimeSurfaceDemoError::SpriteBindFailed; return false; }
    SoftwareRenderer renderer; RenderCamera camera; SoftwareSurfacePresenter surface; if (!renderer.Initialize(config.width,config.height)) { error = TriggerRuntimeSurfaceDemoError::RendererInitializeFailed; return false; } if (!camera.Initialize({RenderCameraMode::Orthographic,{},5.0F,60.0F,static_cast<float>(config.width)/static_cast<float>(config.height),0.1F,10.0F})) { error = TriggerRuntimeSurfaceDemoError::CameraInitializeFailed; return false; } if (!surface.Initialize({config.width,config.height,config.hiddenSurface})) { error = TriggerRuntimeSurfaceDemoError::SurfaceInitializeFailed; return false; }
    constexpr uint32_t kClear=0xFF101420U; MovementAuthorityGate authority; uint32_t entered=0,exited=0;
    for (uint32_t frame=0; frame<config.frames; ++frame) { const KinematicPlanarInput input=frame<3U?KinematicPlanarInput{1.0F,0.0F}:KinematicPlanarInput{}; authority.BeginFrame(); if (!authority.Acquire(actor,MovementAuthority::KinematicRoute)) { error = TriggerRuntimeSurfaceDemoError::AuthorityFailed; return false; } if (!motion.Step(world,actor,input,0.25F) || !world.UpdateTransforms()) { error = TriggerRuntimeSurfaceDemoError::MotionFailed; return false; } if (!poseSync.Sync(world,entities)) { error = TriggerRuntimeSurfaceDemoError::PoseSyncFailed; return false; } physics.Step(entities,1.0F/60.0F); physics.SetEntityLayer(0U,COLLISION_LAYER_STATIC); if (!trigger.Update(physics)) { error = TriggerRuntimeSurfaceDemoError::TriggerUpdateFailed; return false; } entered += static_cast<uint32_t>(trigger.LastDelta().entered.size()); exited += static_cast<uint32_t>(trigger.LastDelta().exited.size()); if (!renderer.Clear(kClear)) { error = TriggerRuntimeSurfaceDemoError::ClearFailed; return false; } SpriteBatch batch; if (!sprites.Queue(world,batch)) { error = TriggerRuntimeSurfaceDemoError::SpriteQueueFailed; return false; } if (!batch.Flush(renderer,camera)) { error = TriggerRuntimeSurfaceDemoError::SpriteFlushFailed; return false; } if (!surface.PumpEvents()) { error = TriggerRuntimeSurfaceDemoError::SurfacePumpFailed; return false; } if (surface.CloseRequested()) { error = TriggerRuntimeSurfaceDemoError::SurfaceCloseRequested; return false; } if (!surface.Present(renderer)) { error = TriggerRuntimeSurfaceDemoError::SurfacePresentFailed; return false; } }
    if (!renderer.WritePpm(config.ppmPath)) { error = TriggerRuntimeSurfaceDemoError::ArtifactWriteFailed; return false; } const Transform3* finalTransform=world.GetTransform(actor); if (finalTransform==nullptr) { error = TriggerRuntimeSurfaceDemoError::TransformFailed; return false; } uint32_t visible=0; for(const uint32_t pixel:renderer.Pixels())if(pixel!=kClear)++visible; receipt={config.frames,static_cast<uint32_t>(surface.PresentedFrameCount()),visible,entered,exited,renderer.FrameHash(),finalTransform->x}; return true;
}
} // namespace NeoEngine
