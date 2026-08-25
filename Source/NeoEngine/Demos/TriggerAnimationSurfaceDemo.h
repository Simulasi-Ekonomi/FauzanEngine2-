#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class TriggerAnimationSurfaceDemoError : uint8_t { None, InvalidConfiguration, TextureImportFailed, WorldCreateFailed, TransformFailed, BodyCreateFailed, PoseBindFailed, MotionInitializeFailed, TriggerInitializeFailed, AnimationInitializeFailed, TintInitializeFailed, SpriteBindFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, AuthorityFailed, MotionFailed, PoseSyncFailed, TriggerUpdateFailed, AnimationTriggerFailed, AnimationUpdateFailed, AnimationSampleFailed, TintResolveFailed, ClearFailed, SpriteQueueFailed, SpriteFlushFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct TriggerAnimationSurfaceDemoConfig { uint32_t width = 256U; uint32_t height = 256U; uint32_t frames = 4U; bool hiddenSurface = true; std::string ppmPath = "trigger_animation_surface_demo.ppm"; };
struct TriggerAnimationSurfaceDemoReceipt { uint32_t renderedFrames = 0U; uint32_t presentedFrames = 0U; uint32_t visiblePixels = 0U; uint32_t enteredEvents = 0U; uint32_t exitedEvents = 0U; uint32_t activeTintFrames = 0U; uint64_t frameHash = 0U; uint64_t tintHash = 0U; float finalX = 0.0F; float finalAnimationSample = 0.0F; };

// Finite runtime proof: kinematic movement remains sole SceneWorld transform writer;
// trigger deltas only select animation/tint and never feed motion, SceneWorld, or XPBD.
bool RunTriggerAnimationSurfaceDemo(const TriggerAnimationSurfaceDemoConfig& config, TriggerAnimationSurfaceDemoReceipt& receipt, TriggerAnimationSurfaceDemoError& error);
} // namespace NeoEngine
