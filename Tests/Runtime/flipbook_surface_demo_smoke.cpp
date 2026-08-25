#include "Demos/FlipbookSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine;
    const char* path = "/tmp/fauzanengine_flipbook_surface_demo.ppm";
    FlipbookSurfaceDemoReceipt receipt{}; FlipbookSurfaceDemoError error = FlipbookSurfaceDemoError::None;
    if (RunFlipbookSurfaceDemo({64U,64U,3U,true,path}, receipt, error) || error != FlipbookSurfaceDemoError::InvalidConfiguration) return 1;
    if (!RunFlipbookSurfaceDemo({64U,64U,4U,true,path}, receipt, error) || error != FlipbookSurfaceDemoError::None) return 1;
    std::ifstream file(path, std::ios::binary); char magic[2]{}; file.read(magic, 2);
    if (!file || magic[0] != 'P' || magic[1] != '6' || receipt.renderedFrames != 4U || receipt.presentedFrames != 4U || receipt.selectedFrames != 4U || receipt.visiblePixels == 0U || receipt.sequenceHash == 0U || receipt.finalFrameHash == 0U) return 1;
    std::printf("FLIPBOOK_SURFACE_DEMO_SMOKE_OK frames=%u selected=%u visible=%u sequence=%llu final=%llu\n", receipt.renderedFrames, receipt.selectedFrames, receipt.visiblePixels, static_cast<unsigned long long>(receipt.sequenceHash), static_cast<unsigned long long>(receipt.finalFrameHash));
    return 0;
}
