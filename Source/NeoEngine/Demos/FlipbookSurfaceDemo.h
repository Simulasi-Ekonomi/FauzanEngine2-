#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {

enum class FlipbookSurfaceDemoError : uint8_t {
    None,
    InvalidConfiguration,
    WorldFailed,
    SpriteFailed,
    SelectorFailed,
    RendererFailed,
    CameraFailed,
    SurfaceFailed,
    RenderFailed,
    PresentFailed,
    TransformChanged,
    ArtifactFailed,
};

struct FlipbookSurfaceDemoConfig {
    uint32_t width = 64U;
    uint32_t height = 64U;
    uint32_t frames = 4U;
    bool hiddenSurface = true;
    std::string ppmPath;
};

struct FlipbookSurfaceDemoReceipt {
    uint32_t renderedFrames = 0U;
    uint32_t presentedFrames = 0U;
    uint32_t visiblePixels = 0U;
    uint64_t sequenceHash = 0U;
    uint64_t finalFrameHash = 0U;
    uint32_t selectedFrames = 0U;
};

bool RunFlipbookSurfaceDemo(const FlipbookSurfaceDemoConfig& config, FlipbookSurfaceDemoReceipt& receipt, FlipbookSurfaceDemoError& error);

} // namespace NeoEngine
