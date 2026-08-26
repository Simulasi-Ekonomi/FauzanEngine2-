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
    std::vector<TextureImportReceipt> setReceipts{{"sentinel",1U,1U,1U,TextureImportFormat::PpmP6}}; const std::vector<TextureImportRequest> set{{"set.a",{},ppm,TextureImportFormat::PpmP6},{"set.b",{},std::vector<uint8_t>{'P','6','\n','1',' ','1','\n','2','5','5','\n',200U,40U,20U},TextureImportFormat::PpmP6}};
    if(!pipeline.ImportSet(registry,textures,set,setReceipts)||pipeline.LastError()!=TextureImportPipelineError::None||setReceipts.size()!=2U||setReceipts[0].assetId!="set.a"||setReceipts[1].assetId!="set.b"||registry.All().size()!=assets+2U||textures.ResourceCount()!=staged+2U)return 1;
    const std::vector<TextureImportReceipt> preservedSet=setReceipts; const size_t setAssets=registry.All().size(),setStaged=textures.ResourceCount(); const std::vector<TextureImportRequest> brokenSet{{"set.c",{},ppm,TextureImportFormat::PpmP6},{"set.d",{},std::vector<uint8_t>{'P','6'},TextureImportFormat::PpmP6}};
    if(pipeline.ImportSet(registry,textures,brokenSet,setReceipts)||pipeline.LastError()!=TextureImportPipelineError::StageFailed||registry.All().size()!=setAssets||textures.ResourceCount()!=setStaged||setReceipts.size()!=preservedSet.size()||setReceipts[0].assetId!=preservedSet[0].assetId||pipeline.ImportSet(registry,textures,{},setReceipts)||pipeline.LastError()!=TextureImportPipelineError::InvalidRequest||setReceipts.size()!=preservedSet.size())return 1;
    std::printf("TEXTURE_IMPORT_PIPELINE_SMOKE_OK registry=1 ready=1 staged=1 atomic=1 batch=2 hash=%llu\n",static_cast<unsigned long long>(receipt.contentHash)); return 0;
}
