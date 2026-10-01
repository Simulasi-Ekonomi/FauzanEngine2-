#include "Runtime/BRDFLut.h"
#include "Runtime/PBREnvironment.h"
#include "Runtime/PBREnvironmentDescriptorSet.h"

#include <cstdint>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
#define REQUIRE(...) do { if (!(__VA_ARGS__)) { std::cerr << "PBR_ENV_REQUIRE_FAIL:" << __FILE__ << ":" << __LINE__ << " expr=" << #__VA_ARGS__ << std::endl; std::abort(); } } while (false)
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
    std::cerr << "PBR_ENV_STAGE:LOAD_BEGIN" << std::endl;
    REQUIRE(environment.LoadHDR(path, config));
    std::cerr << "PBR_ENV_STAGE:LOAD_OK" << std::endl;
    REQUIRE(environment.IsCpuReady());
    REQUIRE(environment.Format() == VK_FORMAT_R16G16B16A16_SFLOAT);
    REQUIRE(environment.EnvironmentFaceSize() == 8);
    REQUIRE(environment.IrradianceFaceSize() == 4);
    REQUIRE(environment.PrefilterFaceSize() == 8);
    REQUIRE(environment.PrefilterMipLevels() == 4);
    REQUIRE(environment.Settings().maxReflectionLod == 3.0f);
    REQUIRE(NeoEngine::ValidatePBRIBLSettings(environment.Settings()));

    std::cerr << "PBR_ENV_STAGE:UPLOAD_BEGIN" << std::endl;
    REQUIRE(environment.UploadToVulkan());
    std::cerr << "PBR_ENV_STAGE:UPLOAD_OK" << std::endl;
    REQUIRE(environment.IsGpuReady());
    REQUIRE(environment.EnvironmentView() != VK_NULL_HANDLE);
    REQUIRE(environment.EnvironmentSampler() != VK_NULL_HANDLE);
    REQUIRE(environment.IrradianceView() != VK_NULL_HANDLE);
    REQUIRE(environment.IrradianceSampler() != VK_NULL_HANDLE);
    REQUIRE(environment.PrefilteredView() != VK_NULL_HANDLE);
    REQUIRE(environment.PrefilteredSampler() != VK_NULL_HANDLE);

    NeoEngine::BRDFLut brdfLut;
    std::cerr << "PBR_ENV_STAGE:BRDF_INIT_BEGIN" << std::endl;
    REQUIRE(brdfLut.Initialize(environment.Device(), environment.PhysicalDevice(),
                              environment.GraphicsQueue(), environment.GraphicsQueueFamily()));
    std::cerr << "PBR_ENV_STAGE:BRDF_INIT_OK" << std::endl;
    REQUIRE(brdfLut.Generate());
    std::cerr << "PBR_ENV_STAGE:BRDF_GENERATE_OK" << std::endl;
    std::cerr << "PBR_ENV_BRDF_STATE:image=" << brdfLut.GetImage()
              << " view=" << brdfLut.GetImageView()
              << " sampler=" << brdfLut.GetSampler()
              << " valid=" << (brdfLut.IsValid() ? 1 : 0) << std::endl;
    REQUIRE(brdfLut.IsValid());
    REQUIRE(brdfLut.GetImage() != VK_NULL_HANDLE);
    REQUIRE(brdfLut.GetImageView() != VK_NULL_HANDLE);
    REQUIRE(brdfLut.GetSampler() != VK_NULL_HANDLE);

    std::cerr << "PBR_ENV_STAGE:DESCRIPTOR_INIT_BEGIN" << std::endl;
    NeoEngine::PBREnvironmentDescriptorSet environmentDescriptors;
    REQUIRE(environmentDescriptors.Initialize(environment.Device()));
    std::cerr << "PBR_ENV_STAGE:DESCRIPTOR_INIT_OK" << std::endl;
    REQUIRE(environmentDescriptors.IsValid());
    VkDescriptorSet environmentSet = environmentDescriptors.AllocateSet();
    std::cerr << "PBR_ENV_STAGE:DESCRIPTOR_ALLOC_RESULT set=" << environmentSet << std::endl;
    REQUIRE(environmentSet != VK_NULL_HANDLE);
    environmentDescriptors.Update(environmentSet, environment,
                                   brdfLut.GetImageView(), brdfLut.GetSampler());
    std::cerr << "PBR_ENV_STAGE:DESCRIPTOR_UPDATE_OK" << std::endl;

    environmentDescriptors.Destroy();
    brdfLut.Destroy();
    environment.Destroy();
    REQUIRE(!environment.IsGpuReady());
    return 0;
}
