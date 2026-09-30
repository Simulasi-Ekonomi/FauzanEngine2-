#include "Runtime/BRDFLut.h"
#include "Runtime/PBREnvironment.h"
#include "Runtime/PBREnvironmentDescriptorSet.h"

#include <cstdint>
#include <fstream>
#include <cstdlib>
#include <string>

namespace {
#define REQUIRE(...) do { if (!(__VA_ARGS__)) std::abort(); } while (false)
void WriteTinyHDR(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    REQUIRE(out);
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
    REQUIRE(environment.LoadHDR(path, config));
    REQUIRE(environment.IsCpuReady());
    REQUIRE(environment.Format() == VK_FORMAT_R16G16B16A16_SFLOAT);
    REQUIRE(environment.EnvironmentFaceSize() == 8);
    REQUIRE(environment.IrradianceFaceSize() == 4);
    REQUIRE(environment.PrefilterFaceSize() == 8);
    REQUIRE(environment.PrefilterMipLevels() == 4);
    REQUIRE(environment.Settings().maxReflectionLod == 3.0f);
    REQUIRE(NeoEngine::ValidatePBRIBLSettings(environment.Settings()));

    REQUIRE(environment.UploadToVulkan());
    REQUIRE(environment.IsGpuReady());
    REQUIRE(environment.EnvironmentView() != VK_NULL_HANDLE);
    REQUIRE(environment.EnvironmentSampler() != VK_NULL_HANDLE);
    REQUIRE(environment.IrradianceView() != VK_NULL_HANDLE);
    REQUIRE(environment.IrradianceSampler() != VK_NULL_HANDLE);
    REQUIRE(environment.PrefilteredView() != VK_NULL_HANDLE);
    REQUIRE(environment.PrefilteredSampler() != VK_NULL_HANDLE);

    NeoEngine::BRDFLut brdfLut;
    REQUIRE(brdfLut.Initialize(environment.Device(), environment.PhysicalDevice(),
                              environment.GraphicsQueue(), environment.GraphicsQueueFamily()));
    REQUIRE(brdfLut.Generate());
    REQUIRE(brdfLut.IsValid());

    NeoEngine::PBREnvironmentDescriptorSet environmentDescriptors;
    REQUIRE(environmentDescriptors.Initialize(environment.Device()));
    REQUIRE(environmentDescriptors.IsValid());
    VkDescriptorSet environmentSet = environmentDescriptors.AllocateSet();
    REQUIRE(environmentSet != VK_NULL_HANDLE);
    environmentDescriptors.Update(environmentSet, environment,
                                   brdfLut.GetImageView(), brdfLut.GetSampler());

    environmentDescriptors.Destroy();
    brdfLut.Destroy();
    environment.Destroy();
    REQUIRE(!environment.IsGpuReady());
    return 0;
}
