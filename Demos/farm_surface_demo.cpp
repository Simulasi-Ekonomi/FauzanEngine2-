#include "Demos/FarmSurfaceDemo.h"

#include <charconv>
#include <cstdio>
#include <string_view>

int main(int argc, char** argv) {
    NeoEngine::FarmSurfaceDemoConfig config{};
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (argument == "--visible") { config.hiddenSurface = false; continue; }
        if (argument == "--frames" && index + 1 < argc) { uint32_t frames = 0; const std::string_view value{argv[++index]}; if (std::from_chars(value.data(), value.data() + value.size(), frames).ec != std::errc{}) return 2; config.frames = frames; continue; }
        if (argument == "--output" && index + 1 < argc) { config.ppmPath = argv[++index]; continue; }
        return 2;
    }
    NeoEngine::FarmSurfaceDemoReceipt receipt{};
    NeoEngine::FarmSurfaceDemoError error{};
    if (!NeoEngine::RunFarmSurfaceDemo(config, receipt, error)) { std::fprintf(stderr, "FARM_SURFACE_DEMO_FAIL error=%u\n", static_cast<unsigned>(error)); return 1; }
    std::printf("FARM_SURFACE_DEMO_OK frames=%u presented=%u buildings=%u npcs=%u hash=%llu ppm=%s\n", receipt.renderedFrames, receipt.presentedFrames, receipt.buildings, receipt.npcs, static_cast<unsigned long long>(receipt.frameHash), config.ppmPath.c_str());
    return 0;
}
