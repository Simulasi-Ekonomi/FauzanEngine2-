#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class TriggerRuntimeSurfaceDemoError : uint8_t { None, InvalidConfiguration, TextureImportFailed, WorldCreateFailed, TransformFailed, BodyCreateFailed, PoseBindFailed, MotionInitializeFailed, TriggerInitializeFailed, SpriteBindFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, AuthorityFailed, MotionFailed, PoseSyncFailed, PhysicsStepFailed, TriggerUpdateFailed, ClearFailed, SpriteQueueFailed, SpriteFlushFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct TriggerRuntimeSurfaceDemoConfig { uint32_t width = 256; uint32_t height = 256; uint32_t frames = 4; bool hiddenSurface = true; std::string ppmPath = "trigger_runtime_surface_demo.ppm"; };
struct TriggerRuntimeSurfaceDemoReceipt { uint32_t renderedFrames = 0; uint32_t presentedFrames = 0; uint32_t visiblePixels = 0; uint32_t enteredEvents = 0; uint32_t exitedEvents = 0; uint64_t frameHash = 0; float finalX = 0.0F; };

// Finite integration proof for kinematic authority -> SceneWorld -> ECS pose mirror ->
// XPBD snapshot -> read-only trigger deltas -> staged sprite rendering.
bool RunTriggerRuntimeSurfaceDemo(const TriggerRuntimeSurfaceDemoConfig& config, TriggerRuntimeSurfaceDemoReceipt& receipt, TriggerRuntimeSurfaceDemoError& error);
} // namespace NeoEngine
