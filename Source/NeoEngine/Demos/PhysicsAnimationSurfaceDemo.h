#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class PhysicsAnimationSurfaceDemoError : uint8_t { None, InvalidConfiguration, TextureImportFailed, WorldCreateFailed, TransformFailed, MotionInitializeFailed, AuthorityFailed, PhysicsBodyFailed, PhysicsVelocityFailed, PhysicsBridgeFailed, TimelineFailed, StateMachineFailed, LocomotionInitializeFailed, AnimationUpdateFailed, AnimationSampleFailed, TintBindingFailed, SpriteBindFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, ClearFailed, SpriteQueueFailed, SpriteFlushFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct PhysicsAnimationSurfaceDemoConfig { uint32_t width = 256; uint32_t height = 256; uint32_t frames = 4; bool hiddenSurface = true; std::string ppmPath = "physics_animation_surface_demo.ppm"; };
struct PhysicsAnimationSurfaceDemoReceipt { uint32_t renderedFrames = 0; uint32_t presentedFrames = 0; uint32_t visiblePixels = 0; uint64_t frameHash = 0; float finalX = 0.0F; float finalAnimationSample = 0.0F; bool endedLocomoting = false; uint32_t physicsTintFrames = 0; uint64_t tintHash = 0; };

// Finite proof: ECS velocity selects animation only; KinematicMotionController
// remains the sole writer of the rendered SceneWorld actor transform.
bool RunPhysicsAnimationSurfaceDemo(const PhysicsAnimationSurfaceDemoConfig& config, PhysicsAnimationSurfaceDemoReceipt& receipt, PhysicsAnimationSurfaceDemoError& error);
} // namespace NeoEngine
