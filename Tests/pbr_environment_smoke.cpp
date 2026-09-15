#include "Runtime/PBREnvironment.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <string>

namespace {
void WriteTinyHDR(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    assert(out);
    out << "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 2 +X 2\n";
    const uint8_t scanline[] = {
        2,2,0,2, 130,128, 130,128, 130,128, 130,129,
        2,2,0,2, 130,64, 130,64, 130,64, 130,128
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
    assert(environment.Format() == VK_FORMAT_R16G16B16A16_SFLOAT);
    assert(environment.EnvironmentFaceSize() == 8);
    assert(environment.IrradianceFaceSize() == 4);
    assert(environment.PrefilterFaceSize() == 8);
    assert(environment.PrefilterMipLevels() == 4);
    assert(environment.Settings().maxReflectionLod == 3.0f);
    assert(NeoEngine::ValidatePBRIBLSettings(environment.Settings()));

    assert(environment.UploadToVulkan());
    assert(environment.IsGpuReady());
    assert(environment.EnvironmentView() != VK_NULL_HANDLE);
    assert(environment.EnvironmentSampler() != VK_NULL_HANDLE);
    assert(environment.IrradianceView() != VK_NULL_HANDLE);
    assert(environment.IrradianceSampler() != VK_NULL_HANDLE);
    assert(environment.PrefilteredView() != VK_NULL_HANDLE);
    assert(environment.PrefilteredSampler() != VK_NULL_HANDLE);

    environment.Destroy();
    assert(!environment.IsGpuReady());
    return 0;
}
