#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class KinematicPreflightSurfaceDemoError : uint8_t { None, InvalidConfiguration, TextureImportFailed, WorldCreateFailed, TransformFailed, ObstacleCreateFailed, PreflightInitializeFailed, MotionInitializeFailed, SpriteBindFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, AuthorityFailed, PreflightFailed, ClearFailed, SpriteQueueFailed, SpriteFlushFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct KinematicPreflightSurfaceDemoConfig { uint32_t width = 256U; uint32_t height = 256U; uint32_t frames = 3U; bool hiddenSurface = true; std::string ppmPath = "kinematic_preflight_surface_demo.ppm"; };
struct KinematicPreflightSurfaceDemoReceipt { uint32_t renderedFrames = 0U; uint32_t presentedFrames = 0U; uint32_t visiblePixels = 0U; uint32_t delegatedMoves = 0U; uint32_t blockedMoves = 0U; uint64_t frameHash = 0U; float finalX = 0.0F; };

// Finite preflight proof: XPBD is only queried; successful transform writes remain inside KinematicMotionController.
bool RunKinematicPreflightSurfaceDemo(const KinematicPreflightSurfaceDemoConfig& config, KinematicPreflightSurfaceDemoReceipt& receipt, KinematicPreflightSurfaceDemoError& error);
} // namespace NeoEngine
