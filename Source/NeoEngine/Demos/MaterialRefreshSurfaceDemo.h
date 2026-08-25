#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class MaterialRefreshSurfaceDemoError : uint8_t { None, InvalidConfiguration, MeshImportFailed, MaterialImportFailed, MaterialRefreshFailed, WorldCreateFailed, TransformFailed, SceneBindFailed, SceneRefreshFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, ClearFailed, SceneDrawFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct MaterialRefreshSurfaceDemoConfig { uint32_t width = 256U; uint32_t height = 256U; uint32_t frames = 2U; bool hiddenSurface = true; std::string ppmPath = "material_refresh_surface_demo.ppm"; };
struct MaterialRefreshSurfaceDemoReceipt { uint32_t renderedFrames = 0U; uint32_t presentedFrames = 0U; uint32_t visiblePixels = 0U; uint64_t beforeFrameHash = 0U; uint64_t afterFrameHash = 0U; uint64_t beforeMaterialHash = 0U; uint64_t afterMaterialHash = 0U; uint32_t beforeRgba = 0U; uint32_t afterRgba = 0U; };

// Explicit caller-invoked in-memory material replacement and staged scene refresh proof.
bool RunMaterialRefreshSurfaceDemo(const MaterialRefreshSurfaceDemoConfig& config, MaterialRefreshSurfaceDemoReceipt& receipt, MaterialRefreshSurfaceDemoError& error);
} // namespace NeoEngine
