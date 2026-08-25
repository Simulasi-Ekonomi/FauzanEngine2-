#include "Demos/MotionAnimationSurfaceDemo.h"

#include "Runtime/AnimationLocomotionBridge.h"
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
bool RunMotionAnimationSurfaceDemo(const MotionAnimationSurfaceDemoConfig& config, MotionAnimationSurfaceDemoReceipt& receipt, MotionAnimationSurfaceDemoError& error) {
    receipt = {}; error = MotionAnimationSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames < 4U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = MotionAnimationSurfaceDemoError::InvalidConfiguration; return false; }
    AssetRegistry registry; TextureStagingStore textures; TextureImportPipeline textureImport; TextureImportReceipt textureReceipt{}; const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',90U,230U,140U};
    if (!textureImport.Import(registry, textures, "motion-demo.sprite", {}, ppm, TextureImportFormat::PpmP6, textureReceipt)) { error = MotionAnimationSurfaceDemoError::TextureImportFailed; return false; }
    SceneWorld world; SceneEntity entity{}; if (!world.Create(entity)) { error = MotionAnimationSurfaceDemoError::WorldCreateFailed; return false; } if (!world.SetTransform(entity, {0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !world.UpdateTransforms()) { error = MotionAnimationSurfaceDemoError::TransformFailed; return false; }
    KinematicMotionController motion; if (!motion.Initialize({2.0F, 0.25F, false})) { error = MotionAnimationSurfaceDemoError::MotionInitializeFailed; return false; }
    AnimationTimeline timeline; if (!timeline.AddTrack("idle", {{0.0F,0.0F},{1.0F,0.0F}}) || !timeline.AddTrack("move", {{0.0F,1.0F},{1.0F,1.0F}})) { error = MotionAnimationSurfaceDemoError::TimelineFailed; return false; }
    AnimationStateMachine animation; if (!animation.AddState({"idle","idle",AnimationPlayback::Loop}) || !animation.AddState({"move","move",AnimationPlayback::Loop}) || !animation.AddTransition({"start_move","idle","move",0.0F}) || !animation.AddTransition({"stop_move","move","idle",0.0F}) || !animation.Start("idle")) { error = MotionAnimationSurfaceDemoError::StateMachineFailed; return false; }
    AnimationLocomotionBridge bridge; if (!bridge.Initialize({"start_move","stop_move",0.01F})) { error = MotionAnimationSurfaceDemoError::BridgeInitializeFailed; return false; }
    const CpuTextureResource* texture = textures.Find("motion-demo.sprite"); SceneSpriteAdapter sprites; if (texture == nullptr || !sprites.AddStaged(entity, *texture, 1.0F, 1.0F, 0, 0, 0xFFFFFFFFU)) { error = MotionAnimationSurfaceDemoError::SpriteBindFailed; return false; }
    SoftwareRenderer renderer; RenderCamera camera; SoftwareSurfacePresenter surface; if (!renderer.Initialize(config.width, config.height)) { error = MotionAnimationSurfaceDemoError::RendererInitializeFailed; return false; } if (!camera.Initialize({RenderCameraMode::Orthographic, {}, 5.0F, 60.0F, static_cast<float>(config.width)/static_cast<float>(config.height), 0.1F, 10.0F})) { error = MotionAnimationSurfaceDemoError::CameraInitializeFailed; return false; } if (!surface.Initialize({config.width,config.height,config.hiddenSurface})) { error = MotionAnimationSurfaceDemoError::SurfaceInitializeFailed; return false; }
    constexpr uint32_t kClear=0xFF101420U; MovementAuthorityGate authority; float sampled = 0.0F;
    for (uint32_t frame=0; frame<config.frames; ++frame) { const KinematicPlanarInput input = frame < 2U ? KinematicPlanarInput{1.0F,0.0F} : KinematicPlanarInput{}; authority.BeginFrame(); if (!authority.Acquire(entity, MovementAuthority::KinematicRoute)) { error = MotionAnimationSurfaceDemoError::AuthorityFailed; return false; } if (!motion.Step(world, entity, input, 1.0F/60.0F) || !world.UpdateTransforms()) { error = MotionAnimationSurfaceDemoError::MotionFailed; return false; } if (!bridge.Apply(input, animation)) { error = MotionAnimationSurfaceDemoError::BridgeApplyFailed; return false; } if (!animation.Update(1.0F/60.0F)) { error = MotionAnimationSurfaceDemoError::AnimationUpdateFailed; return false; } if (!animation.Sample(timeline, sampled)) { error = MotionAnimationSurfaceDemoError::AnimationSampleFailed; return false; } if (!renderer.Clear(kClear)) { error = MotionAnimationSurfaceDemoError::ClearFailed; return false; } SpriteBatch batch; if (!sprites.Queue(world,batch)) { error = MotionAnimationSurfaceDemoError::SpriteQueueFailed; return false; } if (!batch.Flush(renderer,camera)) { error = MotionAnimationSurfaceDemoError::SpriteFlushFailed; return false; } if (!surface.PumpEvents()) { error = MotionAnimationSurfaceDemoError::SurfacePumpFailed; return false; } if (surface.CloseRequested()) { error = MotionAnimationSurfaceDemoError::SurfaceCloseRequested; return false; } if (!surface.Present(renderer)) { error = MotionAnimationSurfaceDemoError::SurfacePresentFailed; return false; } }
    if (!renderer.WritePpm(config.ppmPath)) { error = MotionAnimationSurfaceDemoError::ArtifactWriteFailed; return false; } const Transform3* transform=world.GetTransform(entity); if (transform==nullptr) { error = MotionAnimationSurfaceDemoError::TransformFailed; return false; } uint32_t visible=0; for (const uint32_t pixel:renderer.Pixels()) if(pixel!=kClear)++visible; receipt={config.frames,static_cast<uint32_t>(surface.PresentedFrameCount()),visible,renderer.FrameHash(),transform->x,sampled,bridge.IsLocomoting()}; return true;
}
} // namespace NeoEngine
