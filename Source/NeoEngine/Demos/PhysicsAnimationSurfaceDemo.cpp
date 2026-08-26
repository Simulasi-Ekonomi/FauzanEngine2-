#include "Demos/PhysicsAnimationSurfaceDemo.h"

#include "Runtime/AnimationLocomotionBridge.h"
#include "Runtime/AnimationSpriteTintBinding.h"
#include "Runtime/KinematicMotionController.h"
#include "Runtime/MovementAuthority.h"
#include "Runtime/PhysicsAnimationLocomotionBridge.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/SpriteBatch.h"
#include "Runtime/TextureImportPipeline.h"

#include <vector>

namespace NeoEngine {
bool RunPhysicsAnimationSurfaceDemo(const PhysicsAnimationSurfaceDemoConfig& config, PhysicsAnimationSurfaceDemoReceipt& receipt, PhysicsAnimationSurfaceDemoError& error) {
    receipt = {}; error = PhysicsAnimationSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames < 4U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = PhysicsAnimationSurfaceDemoError::InvalidConfiguration; return false; }
    AssetRegistry registry; TextureStagingStore textures; TextureImportPipeline textureImport; TextureImportReceipt textureReceipt{}; const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',90U,230U,140U};
    if (!textureImport.Import(registry, textures, "physics-animation-demo.sprite", {}, ppm, TextureImportFormat::PpmP6, textureReceipt)) { error = PhysicsAnimationSurfaceDemoError::TextureImportFailed; return false; }
    SceneWorld world; SceneEntity entity{}; if (!world.Create(entity)) { error = PhysicsAnimationSurfaceDemoError::WorldCreateFailed; return false; } if (!world.SetTransform(entity, {0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !world.UpdateTransforms()) { error = PhysicsAnimationSurfaceDemoError::TransformFailed; return false; }
    KinematicMotionController motion; if (!motion.Initialize({2.0F, 0.25F, false})) { error = PhysicsAnimationSurfaceDemoError::MotionInitializeFailed; return false; }
    ArchetypeManager entities; GameplayPhysicsBodyBuilder bodies; EntityID body{}; if (!bodies.CreateCircleBody(entities, {GameplayPhysicsBodyType::Dynamic, 0.0F, 0.0F, 0.0F, 0.0F, 0.5F, 1.0F}, body)) { error = PhysicsAnimationSurfaceDemoError::PhysicsBodyFailed; return false; }
    AnimationTimeline timeline; if (!timeline.AddTrack("idle", {{0.0F,0.0F},{1.0F,0.0F}}) || !timeline.AddTrack("move", {{0.0F,1.0F},{1.0F,1.0F}})) { error = PhysicsAnimationSurfaceDemoError::TimelineFailed; return false; }
    AnimationStateMachine animation; if (!animation.AddState({"idle","idle",AnimationPlayback::Loop}) || !animation.AddState({"move","move",AnimationPlayback::Loop}) || !animation.AddTransition({"start_move","idle","move",0.0F}) || !animation.AddTransition({"stop_move","move","idle",0.0F}) || !animation.Start("idle")) { error = PhysicsAnimationSurfaceDemoError::StateMachineFailed; return false; }
    AnimationLocomotionBridge locomotion; if (!locomotion.Initialize({"start_move","stop_move",0.01F})) { error = PhysicsAnimationSurfaceDemoError::LocomotionInitializeFailed; return false; } PhysicsAnimationLocomotionBridge physicsBridge; AnimationSpriteTintBinding tintBinding; if (!tintBinding.Initialize({0xFFFFFFFFU,0xFF80D0FFU})) { error = PhysicsAnimationSurfaceDemoError::TintBindingFailed; return false; }
    const CpuTextureResource* texture = textures.Find("physics-animation-demo.sprite"); SceneSpriteAdapter sprites; if (texture == nullptr || !sprites.AddStaged(entity, *texture, 1.0F, 1.0F, 0, 0, 0xFFFFFFFFU)) { error = PhysicsAnimationSurfaceDemoError::SpriteBindFailed; return false; }
    SoftwareRenderer renderer; RenderCamera camera; SoftwareSurfacePresenter surface; if (!renderer.Initialize(config.width, config.height)) { error = PhysicsAnimationSurfaceDemoError::RendererInitializeFailed; return false; } if (!camera.Initialize({RenderCameraMode::Orthographic, {}, 5.0F, 60.0F, static_cast<float>(config.width)/static_cast<float>(config.height), 0.1F, 10.0F})) { error = PhysicsAnimationSurfaceDemoError::CameraInitializeFailed; return false; } if (!surface.Initialize({config.width,config.height,config.hiddenSurface})) { error = PhysicsAnimationSurfaceDemoError::SurfaceInitializeFailed; return false; }
    constexpr uint32_t kClear=0xFF101420U; MovementAuthorityGate authority; float sampled = 0.0F; uint32_t physicsTintFrames=0U; uint64_t tintHash=1469598103934665603ULL;
    for (uint32_t frame=0; frame<config.frames; ++frame) { const KinematicPlanarInput input = frame < 2U ? KinematicPlanarInput{1.0F,0.0F} : KinematicPlanarInput{}; authority.BeginFrame(); if (!authority.Acquire(entity, MovementAuthority::KinematicRoute)) { error = PhysicsAnimationSurfaceDemoError::AuthorityFailed; return false; } if (!bodies.SetDynamicPlanarVelocity(entities, body, input.x * 2.0F, input.z * 2.0F)) { error = PhysicsAnimationSurfaceDemoError::PhysicsVelocityFailed; return false; } if (!motion.Step(world, entity, input, 1.0F/60.0F) || !world.UpdateTransforms()) { error = PhysicsAnimationSurfaceDemoError::TransformFailed; return false; } if (!physicsBridge.Apply(entities, body, bodies, locomotion, animation)) { error = PhysicsAnimationSurfaceDemoError::PhysicsBridgeFailed; return false; } if (!animation.Update(1.0F/60.0F) || !animation.Sample(timeline, sampled)) { error = PhysicsAnimationSurfaceDemoError::AnimationUpdateFailed; return false; } uint32_t frameTint=0U; if (!tintBinding.Resolve(sampled,frameTint)) { error = PhysicsAnimationSurfaceDemoError::TintBindingFailed; return false; } if(sampled>0.0F)++physicsTintFrames; tintHash=(tintHash^frameTint)*1099511628211ULL; if (!renderer.Clear(kClear)) { error = PhysicsAnimationSurfaceDemoError::ClearFailed; return false; } SpriteBatch batch; if (!sprites.QueueTinted(world,batch,frameTint)) { error = PhysicsAnimationSurfaceDemoError::SpriteQueueFailed; return false; } if (!batch.Flush(renderer,camera)) { error = PhysicsAnimationSurfaceDemoError::SpriteFlushFailed; return false; } if (!surface.PumpEvents()) { error = PhysicsAnimationSurfaceDemoError::SurfacePumpFailed; return false; } if (surface.CloseRequested()) { error = PhysicsAnimationSurfaceDemoError::SurfaceCloseRequested; return false; } if (!surface.Present(renderer)) { error = PhysicsAnimationSurfaceDemoError::SurfacePresentFailed; return false; } }
    if (!renderer.WritePpm(config.ppmPath)) { error = PhysicsAnimationSurfaceDemoError::ArtifactWriteFailed; return false; } const Transform3* transform=world.GetTransform(entity); if (transform==nullptr) { error = PhysicsAnimationSurfaceDemoError::TransformFailed; return false; } uint32_t visible=0; for (const uint32_t pixel:renderer.Pixels()) if(pixel!=kClear)++visible; receipt={config.frames,static_cast<uint32_t>(surface.PresentedFrameCount()),visible,renderer.FrameHash(),transform->x,sampled,locomotion.IsLocomoting(),physicsTintFrames,tintHash}; return true;
}
} // namespace NeoEngine
