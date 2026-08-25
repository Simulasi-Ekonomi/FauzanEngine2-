#include "Demos/TriggerAnimationSurfaceDemo.h"

#include "Runtime/AnimationSpriteTintBinding.h"
#include "Runtime/AnimationStateMachine.h"
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
bool RunTriggerAnimationSurfaceDemo(const TriggerAnimationSurfaceDemoConfig& config, TriggerAnimationSurfaceDemoReceipt& receipt, TriggerAnimationSurfaceDemoError& error) {
    receipt = {}; error = TriggerAnimationSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames < 4U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error=TriggerAnimationSurfaceDemoError::InvalidConfiguration; return false; }
    AssetRegistry registry; TextureStagingStore textures; TextureImportPipeline textureImport; TextureImportReceipt textureReceipt{}; const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',250U,196U,68U};
    if(!textureImport.Import(registry,textures,"trigger-animation.sprite",{},ppm,TextureImportFormat::PpmP6,textureReceipt)){error=TriggerAnimationSurfaceDemoError::TextureImportFailed;return false;}
    SceneWorld world; SceneEntity actor{}; if(!world.Create(actor)){error=TriggerAnimationSurfaceDemoError::WorldCreateFailed;return false;} if(!world.SetTransform(actor,{-0.75F,0.0F,3.0F,0.0F,0.0F,0.0F,1.0F,1.0F,1.0F})||!world.UpdateTransforms()){error=TriggerAnimationSurfaceDemoError::TransformFailed;return false;}
    ArchetypeManager entities; GameplayPhysicsBodyBuilder bodies; EntityID actorBody=0; if(!bodies.CreateCircleBody(entities,{GameplayPhysicsBodyType::Static,0.0F,0.0F,0.0F,0.0F,0.12F,0.0F},actorBody)){error=TriggerAnimationSurfaceDemoError::BodyCreateFailed;return false;}
    ScenePhysicsPoseSync poseSync; if(!poseSync.Bind(actor,actorBody)){error=TriggerAnimationSurfaceDemoError::PoseBindFailed;return false;} XPBDPhysicsSystem physics; physics.Step(entities,1.0F/60.0F); physics.SetEntityLayer(0U,COLLISION_LAYER_STATIC);
    KinematicMotionController motion; if(!motion.Initialize({2.0F,0.25F,false})){error=TriggerAnimationSurfaceDemoError::MotionInitializeFailed;return false;} GameplayTriggerTracker trigger; if(!trigger.Initialize({0.0F,3.0F,0.30F,COLLISION_LAYER_STATIC})){error=TriggerAnimationSurfaceDemoError::TriggerInitializeFailed;return false;}
    AnimationTimeline timeline; AnimationStateMachine animation; if(!timeline.AddTrack("idle",{{0.0F,0.0F},{1.0F,0.0F}})||!timeline.AddTrack("active",{{0.0F,1.0F},{1.0F,1.0F}})||!animation.AddState({"idle","idle",AnimationPlayback::Loop})||!animation.AddState({"active","active",AnimationPlayback::Loop})||!animation.AddTransition({"trigger_enter","idle","active",0.0F})||!animation.AddTransition({"trigger_exit","active","idle",0.0F})||!animation.Start("idle")){error=TriggerAnimationSurfaceDemoError::AnimationInitializeFailed;return false;}
    AnimationSpriteTintBinding tint; if(!tint.Initialize({0xFFFFFFFFU,0xFFFF70B0U})){error=TriggerAnimationSurfaceDemoError::TintInitializeFailed;return false;}
    const CpuTextureResource* texture=textures.Find("trigger-animation.sprite"); SceneSpriteAdapter sprites; if(texture==nullptr||!sprites.AddStaged(actor,*texture,1.0F,1.0F,0,0,0xFFFFFFFFU)){error=TriggerAnimationSurfaceDemoError::SpriteBindFailed;return false;}
    SoftwareRenderer renderer; RenderCamera camera; SoftwareSurfacePresenter surface; if(!renderer.Initialize(config.width,config.height)){error=TriggerAnimationSurfaceDemoError::RendererInitializeFailed;return false;} if(!camera.Initialize({RenderCameraMode::Orthographic,{},5.0F,60.0F,static_cast<float>(config.width)/static_cast<float>(config.height),0.1F,10.0F})){error=TriggerAnimationSurfaceDemoError::CameraInitializeFailed;return false;} if(!surface.Initialize({config.width,config.height,config.hiddenSurface})){error=TriggerAnimationSurfaceDemoError::SurfaceInitializeFailed;return false;}
    constexpr uint32_t kClear=0xFF101420U; MovementAuthorityGate authority; uint32_t entered=0U,exited=0U,activeTintFrames=0U; uint64_t tintHash=1469598103934665603ULL; float sample=0.0F;
    for(uint32_t frame=0U;frame<config.frames;++frame){const KinematicPlanarInput input=frame<3U?KinematicPlanarInput{1.0F,0.0F}:KinematicPlanarInput{};authority.BeginFrame();if(!authority.Acquire(actor,MovementAuthority::KinematicRoute)){error=TriggerAnimationSurfaceDemoError::AuthorityFailed;return false;}if(!motion.Step(world,actor,input,0.25F)||!world.UpdateTransforms()){error=TriggerAnimationSurfaceDemoError::MotionFailed;return false;}if(!poseSync.Sync(world,entities)){error=TriggerAnimationSurfaceDemoError::PoseSyncFailed;return false;}physics.Step(entities,1.0F/60.0F);physics.SetEntityLayer(0U,COLLISION_LAYER_STATIC);if(!trigger.Update(physics)){error=TriggerAnimationSurfaceDemoError::TriggerUpdateFailed;return false;}const auto& delta=trigger.LastDelta();entered+=static_cast<uint32_t>(delta.entered.size());exited+=static_cast<uint32_t>(delta.exited.size());if(!delta.entered.empty()&&!animation.Trigger("trigger_enter")){error=TriggerAnimationSurfaceDemoError::AnimationTriggerFailed;return false;}if(!delta.exited.empty()&&!animation.Trigger("trigger_exit")){error=TriggerAnimationSurfaceDemoError::AnimationTriggerFailed;return false;}if(!animation.Update(0.25F)){error=TriggerAnimationSurfaceDemoError::AnimationUpdateFailed;return false;}if(!animation.Sample(timeline,sample)){error=TriggerAnimationSurfaceDemoError::AnimationSampleFailed;return false;}uint32_t frameTint=0U;if(!tint.Resolve(sample,frameTint)){error=TriggerAnimationSurfaceDemoError::TintResolveFailed;return false;}if(sample>0.0F)++activeTintFrames;tintHash=(tintHash^frameTint)*1099511628211ULL;if(!renderer.Clear(kClear)){error=TriggerAnimationSurfaceDemoError::ClearFailed;return false;}SpriteBatch batch;if(!sprites.QueueTinted(world,batch,frameTint)){error=TriggerAnimationSurfaceDemoError::SpriteQueueFailed;return false;}if(!batch.Flush(renderer,camera)){error=TriggerAnimationSurfaceDemoError::SpriteFlushFailed;return false;}if(!surface.PumpEvents()){error=TriggerAnimationSurfaceDemoError::SurfacePumpFailed;return false;}if(surface.CloseRequested()){error=TriggerAnimationSurfaceDemoError::SurfaceCloseRequested;return false;}if(!surface.Present(renderer)){error=TriggerAnimationSurfaceDemoError::SurfacePresentFailed;return false;}}
    if(!renderer.WritePpm(config.ppmPath)){error=TriggerAnimationSurfaceDemoError::ArtifactWriteFailed;return false;}const Transform3* finalTransform=world.GetTransform(actor);if(finalTransform==nullptr){error=TriggerAnimationSurfaceDemoError::TransformFailed;return false;}uint32_t visible=0U;for(uint32_t pixel:renderer.Pixels())if(pixel!=kClear)++visible;receipt={config.frames,static_cast<uint32_t>(surface.PresentedFrameCount()),visible,entered,exited,activeTintFrames,renderer.FrameHash(),tintHash,finalTransform->x,sample};return true;
}
} // namespace NeoEngine
