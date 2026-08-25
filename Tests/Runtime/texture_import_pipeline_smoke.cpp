#include "Runtime/TextureImportPipeline.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',20U,40U,200U}; AssetRegistry registry; TextureStagingStore textures; TextureImportPipeline pipeline; TextureImportReceipt receipt{};
    if(!pipeline.Import(registry,textures,"pipeline.ppm",{},ppm,TextureImportFormat::PpmP6,receipt)||pipeline.LastError()!=TextureImportPipelineError::None||receipt.assetId!="pipeline.ppm"||receipt.width!=1U||receipt.height!=1U||registry.Find("pipeline.ppm")==nullptr||registry.Find("pipeline.ppm")->state!=AssetState::Ready||textures.Find("pipeline.ppm")==nullptr)return 1;
    const TextureImportReceipt preserved=receipt; const size_t assets=registry.All().size(), staged=textures.ResourceCount();
    if(pipeline.Import(registry,textures,"broken.ppm",{},std::vector<uint8_t>{'P','6'},TextureImportFormat::PpmP6,receipt)||pipeline.LastError()!=TextureImportPipelineError::StageFailed||registry.All().size()!=assets||textures.ResourceCount()!=staged||receipt.assetId!=preserved.assetId)return 1;
    if(pipeline.Import(registry,textures,"pipeline.ppm",{},ppm,TextureImportFormat::PpmP6,receipt)||pipeline.LastError()!=TextureImportPipelineError::RegistryImportFailed||registry.All().size()!=assets||textures.ResourceCount()!=staged||receipt.assetId!=preserved.assetId)return 1;
    if(pipeline.Import(registry,textures,"",{},ppm,TextureImportFormat::PpmP6,receipt)||pipeline.LastError()!=TextureImportPipelineError::InvalidRequest||registry.All().size()!=assets||textures.ResourceCount()!=staged)return 1;
    std::printf("TEXTURE_IMPORT_PIPELINE_SMOKE_OK registry=1 ready=1 staged=1 atomic=1 hash=%llu\n",static_cast<unsigned long long>(receipt.contentHash)); return 0;
}
