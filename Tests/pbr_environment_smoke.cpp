#include "Runtime/PBREnvironment.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace {
void WriteTinyHDR(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    assert(out);
    out << "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 2 +X 2\n";
    const uint8_t scanline[] = {
        2, 2, 0, 2,
        1, 128, 128, 129, 2, 128, 128, 129,
        1, 128, 128, 129, 2, 128, 128, 129,
        2, 2, 0, 2,
        1, 128, 128, 130, 2, 128, 128, 130,
        1, 128, 128, 130, 2, 128, 128, 130
    };
    out.write(reinterpret_cast<const char*>(scanline), sizeof(scanline));
}
}

int main() {
    const std::string path = "/tmp/neo_pbr_environment_smoke.hdr";
    WriteTinyHDR(path);

    NeoEngine::PBREnvironment environment;
    NeoEngine::PBREnvironmentConfig config{};
    config.environmentFaceSize = 8;
    config.irradianceFaceSize = 4;
    config.prefilterFaceSize = 8;
    config.prefilterSamples = 8;
    assert(environment.LoadHDR(path, config));
    assert(environment.IsCpuReady());
    assert(environment.EnvironmentFaceSize() == 8);
    assert(environment.IrradianceFaceSize() == 4);
    assert(environment.PrefilterFaceSize() == 8);
    assert(environment.PrefilterMipLevels() == 4);
    assert(environment.Settings().maxReflectionLod == 3.0f);

    const auto settings = environment.Settings();
    assert(NeoEngine::ValidatePBRIBLSettings(settings));
    return 0;
}
