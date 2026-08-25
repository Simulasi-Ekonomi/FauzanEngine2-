#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class MeshSurfaceDemoError : uint8_t { None, InvalidConfiguration, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, ClearFailed, MeshDrawFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct MeshSurfaceDemoConfig { uint32_t width = 256; uint32_t height = 256; uint32_t frames = 8; bool hiddenSurface = true; std::string ppmPath = "mesh_surface_demo.ppm"; };
struct MeshSurfaceDemoReceipt { uint32_t renderedFrames = 0; uint32_t presentedFrames = 0; uint32_t visiblePixels = 0; uint64_t frameHash = 0; };

// Runs a finite textured 3D mesh scene through canonical software rendering and SDL presentation.
// It does not provide input, persistent host loop, gameplay authority, or release packaging.
bool RunMeshSurfaceDemo(const MeshSurfaceDemoConfig& config, MeshSurfaceDemoReceipt& receipt, MeshSurfaceDemoError& error);
} // namespace NeoEngine
