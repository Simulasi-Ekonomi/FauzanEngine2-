#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class FarmSurfaceDemoError : uint8_t { None, InvalidConfiguration, RuntimeInitializeFailed, FarmSetupFailed, FrameFailed, ArtifactWriteFailed, ShutdownFailed };
struct FarmSurfaceDemoConfig { uint32_t width = 256; uint32_t height = 256; uint32_t frames = 8; bool hiddenSurface = true; std::string ppmPath = "farm_surface_demo.ppm"; };
struct FarmSurfaceDemoReceipt { uint32_t renderedFrames = 0; uint64_t frameHash = 0; uint32_t presentedFrames = 0; uint32_t buildings = 0; uint32_t npcs = 0; };

// Runs a finite, inspectable Farm scene using only canonical runtime APIs.
// It does not create a persistent game loop, external authority, or save data.
bool RunFarmSurfaceDemo(const FarmSurfaceDemoConfig& config, FarmSurfaceDemoReceipt& receipt, FarmSurfaceDemoError& error);
} // namespace NeoEngine
