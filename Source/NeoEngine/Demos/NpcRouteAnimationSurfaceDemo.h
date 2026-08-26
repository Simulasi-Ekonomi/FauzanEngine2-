#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class NpcRouteAnimationSurfaceDemoError : uint8_t { None, InvalidConfiguration, TextureImportFailed, WorldCreateFailed, TransformFailed, NavigationFailed, RouteFailed, ControllerFailed, PhysicsBodyFailed, PhysicsVelocityFailed, PhysicsBridgeFailed, TimelineFailed, StateMachineFailed, LocomotionInitializeFailed, AnimationUpdateFailed, AnimationSampleFailed, TintBindingFailed, SpriteBindFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, ClearFailed, SpriteQueueFailed, SpriteFlushFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct NpcRouteAnimationSurfaceDemoConfig { uint32_t width = 256; uint32_t height = 256; uint32_t frames = 5; bool hiddenSurface = true; std::string ppmPath = "npc_route_animation_surface_demo.ppm"; };
struct NpcRouteAnimationSurfaceDemoReceipt { uint32_t renderedFrames = 0; uint32_t presentedFrames = 0; uint32_t visiblePixels = 0; uint64_t frameHash = 0; float finalX = 0.0F; float finalZ = 0.0F; float finalAnimationSample = 0.0F; bool reachedGoal = false; bool endedLocomoting = false; uint32_t routeTintFrames = 0; uint64_t tintHash = 0; };

// Finite NPC proof: GridRouteFollower delegates all scene movement through a
// guarded KinematicMotionController; its intent supplies ECS velocity for animation only.
bool RunNpcRouteAnimationSurfaceDemo(const NpcRouteAnimationSurfaceDemoConfig& config, NpcRouteAnimationSurfaceDemoReceipt& receipt, NpcRouteAnimationSurfaceDemoError& error);
} // namespace NeoEngine
