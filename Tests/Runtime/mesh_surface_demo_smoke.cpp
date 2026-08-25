#include "Demos/MeshSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine;
    const char* output = "/tmp/fauzanengine_mesh_surface_demo_smoke.ppm";
    MeshSurfaceDemoConfig invalid{};
    invalid.width = 16U;
    MeshSurfaceDemoReceipt receipt{};
    MeshSurfaceDemoError error{};
    if (RunMeshSurfaceDemo(invalid, receipt, error) || error != MeshSurfaceDemoError::InvalidConfiguration) return 1;
    MeshSurfaceDemoConfig config{};
    config.width = 96U;
    config.height = 64U;
    config.frames = 4U;
    config.hiddenSurface = true;
    config.ppmPath = output;
    if (!RunMeshSurfaceDemo(config, receipt, error) || receipt.renderedFrames != 4U || receipt.presentedFrames != 4U || receipt.visiblePixels == 0U || receipt.frameHash == 0U) return 1;
    std::ifstream artifact(output, std::ios::binary);
    char header[2]{};
    if (!artifact.read(header, 2) || header[0] != 'P' || header[1] != '6') return 1;
    artifact.close();
    std::remove(output);
    std::printf("MESH_SURFACE_DEMO_SMOKE_OK frames=%u presented=%u pixels=%u hash=%llu\n", receipt.renderedFrames, receipt.presentedFrames, receipt.visiblePixels, static_cast<unsigned long long>(receipt.frameHash));
    return 0;
}
