#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class HybridSurfaceDemoError : uint8_t { None, InvalidConfiguration, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, ClearFailed, MeshDrawFailed, SpriteDrawFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct HybridSurfaceDemoConfig { uint32_t width = 256; uint32_t height = 256; uint32_t frames = 4; bool hiddenSurface = true; std::string ppmPath = "hybrid_surface_demo.ppm"; };
struct HybridSurfaceDemoReceipt { uint32_t renderedFrames = 0; uint32_t presentedFrames = 0; uint32_t visiblePixels = 0; uint64_t frameHash = 0; };

// Runs a finite software-rendered mesh plus billboard-sprite proof through SDL presentation.
// It has no persistent host loop, input, gameplay, authority, GPU, or release behavior.
bool RunHybridSurfaceDemo(const HybridSurfaceDemoConfig& config, HybridSurfaceDemoReceipt& receipt, HybridSurfaceDemoError& error);
} // namespace NeoEngine
