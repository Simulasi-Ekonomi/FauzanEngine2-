#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class MaterialImportSurfaceDemoError : uint8_t { None, InvalidConfiguration, TextureImportFailed, MeshImportFailed, MaterialImportFailed, WorldCreateFailed, TransformFailed, SceneBindFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, ClearFailed, SceneDrawFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct MaterialImportSurfaceDemoConfig { uint32_t width = 256U; uint32_t height = 256U; uint32_t frames = 4U; bool hiddenSurface = true; std::string ppmPath = "material_import_surface_demo.ppm"; };
struct MaterialImportSurfaceDemoReceipt { uint32_t renderedFrames = 0U; uint32_t presentedFrames = 0U; uint32_t visiblePixels = 0U; uint64_t frameHash = 0U; uint64_t textureHash = 0U; uint64_t meshHash = 0U; uint64_t materialHash = 0U; uint32_t materialRgba = 0U; };

// Finite software proof only: PPM/OBJ/MTL candidate pipelines stage CPU resources,
// then a copy-on-register SceneMeshAdapter renders them through an opt-in SDL surface.
bool RunMaterialImportSurfaceDemo(const MaterialImportSurfaceDemoConfig& config, MaterialImportSurfaceDemoReceipt& receipt, MaterialImportSurfaceDemoError& error);
} // namespace NeoEngine
