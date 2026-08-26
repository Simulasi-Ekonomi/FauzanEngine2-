#include "Demos/PhysicsAnimationSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine;
    PhysicsAnimationSurfaceDemoReceipt receipt{}; PhysicsAnimationSurfaceDemoError error = PhysicsAnimationSurfaceDemoError::None;
    if (RunPhysicsAnimationSurfaceDemo({31U, 96U, 4U, true, "/tmp/physics_animation_surface_invalid.ppm"}, receipt, error) || error != PhysicsAnimationSurfaceDemoError::InvalidConfiguration) return 1;
    const char* path = "/tmp/physics_animation_surface_demo.ppm";
    if (!RunPhysicsAnimationSurfaceDemo({96U, 96U, 4U, true, path}, receipt, error) || error != PhysicsAnimationSurfaceDemoError::None || receipt.renderedFrames != 4U || receipt.presentedFrames != 4U || receipt.visiblePixels == 0U || receipt.frameHash == 0U || receipt.finalX <= 0.0F || receipt.finalAnimationSample != 0.0F || receipt.endedLocomoting || receipt.physicsTintFrames != 2U || receipt.tintHash == 0U) return 1;
    std::ifstream input(path, std::ios::binary); char magic[2]{}; if (!input.read(magic, 2) || magic[0] != 'P' || magic[1] != '6') return 1;
    std::printf("PHYSICS_ANIMATION_SURFACE_DEMO_SMOKE_OK frames=%u tint=%u x=%.3f hash=%llu\n", receipt.renderedFrames, receipt.physicsTintFrames, receipt.finalX, static_cast<unsigned long long>(receipt.frameHash));
    return 0;
}
