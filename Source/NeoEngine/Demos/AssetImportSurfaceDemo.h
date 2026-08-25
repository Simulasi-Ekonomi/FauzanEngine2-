#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class AssetImportSurfaceDemoError : uint8_t { None, InvalidConfiguration, TextureImportFailed, MeshImportFailed, RendererInitializeFailed, CameraInitializeFailed, SurfaceInitializeFailed, ClearFailed, MeshDrawFailed, SurfacePumpFailed, SurfaceCloseRequested, SurfacePresentFailed, ArtifactWriteFailed };
struct AssetImportSurfaceDemoConfig { uint32_t width = 256; uint32_t height = 256; uint32_t frames = 8; bool hiddenSurface = true; std::string ppmPath = "asset_import_surface_demo.ppm"; };
struct AssetImportSurfaceDemoReceipt { uint32_t renderedFrames = 0; uint32_t presentedFrames = 0; uint32_t visiblePixels = 0; uint64_t frameHash = 0; uint64_t meshHash = 0; uint64_t textureHash = 0; };

// Finite proof of canonical in-memory texture/mesh import flowing to staged CPU
// resources, software mesh rendering, SDL presentation, and a PPM artifact.
bool RunAssetImportSurfaceDemo(const AssetImportSurfaceDemoConfig& config, AssetImportSurfaceDemoReceipt& receipt, AssetImportSurfaceDemoError& error);
} // namespace NeoEngine
