#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class MotionAnimationSurfaceDemoError : uint8_t { None, InvalidConfiguration, TextureImportFailed, WorldCreateFailed, TransformFailed, MotionInitializeFailed, AuthorityFailed, MotionFailed, TimelineFailed, StateMachineFailed, BridgeInitializeFailed, BridgeApplyFailed, AnimationUpdateFailed, AnimationSampleFailed, TintBindingFailed, SpriteBindFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, ClearFailed, SpriteQueueFailed, SpriteFlushFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct MotionAnimationSurfaceDemoConfig { uint32_t width = 256; uint32_t height = 256; uint32_t frames = 4; bool hiddenSurface = true; std::string ppmPath = "motion_animation_surface_demo.ppm"; };
struct MotionAnimationSurfaceDemoReceipt { uint32_t renderedFrames = 0; uint32_t presentedFrames = 0; uint32_t visiblePixels = 0; uint64_t frameHash = 0; float finalX = 0.0F; float sampledAnimation = 0.0F; bool endedLocomoting = false; uint32_t locomotionTintFrames = 0; uint64_t tintHash = 0; };

// Finite runtime proof: kinematic motion is the only transform writer, while
// animation receives input as trigger-only state selection and emits frame-local sprite tint while render reads world state.
bool RunMotionAnimationSurfaceDemo(const MotionAnimationSurfaceDemoConfig& config, MotionAnimationSurfaceDemoReceipt& receipt, MotionAnimationSurfaceDemoError& error);
} // namespace NeoEngine
