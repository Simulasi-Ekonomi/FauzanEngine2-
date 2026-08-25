#include "Demos/FarmSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine;
    FarmSurfaceDemoReceipt receipt{};
    FarmSurfaceDemoError error{};
    const std::string output = "farm_surface_demo_smoke.ppm";
    if (RunFarmSurfaceDemo({64, 64, 4, true, output}, receipt, error) == false || error != FarmSurfaceDemoError::None || receipt.renderedFrames != 4U || receipt.presentedFrames != 4U || receipt.frameHash == 0U || receipt.buildings != 1U || receipt.npcs != 5U) return 1;
    std::ifstream artifact(output, std::ios::binary);
    char magic[2]{};
    artifact.read(magic, 2);
    artifact.close();
    std::remove(output.c_str());
    FarmSurfaceDemoReceipt rejectedReceipt{};
    FarmSurfaceDemoError rejectedError{};
    if (RunFarmSurfaceDemo({16, 64, 1, true, "invalid.ppm"}, rejectedReceipt, rejectedError) || rejectedError != FarmSurfaceDemoError::InvalidConfiguration || magic[0] != 'P' || magic[1] != '6') return 1;
    std::printf("FARM_SURFACE_DEMO_SMOKE_OK frames=%u presented=%u ppm=1 hash=%llu\n", receipt.renderedFrames, receipt.presentedFrames, static_cast<unsigned long long>(receipt.frameHash));
    return 0;
}
