#include "Runtime/MaterialImportPipeline.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    AssetRegistry registry; MaterialStagingStore materials; MaterialImportPipeline pipeline; MaterialImportReceipt receipt{};
    const std::vector<uint8_t> valid{'n','e','w','m','t','l',' ','f','a','r','m','\n','K','d',' ','0','.','2',' ','0','.','4',' ','0','.','6','\n','d',' ','0','.','5','\n'};
    if (!pipeline.ImportMtl(registry, materials, "farm.material", {}, valid, "farm", receipt) || pipeline.LastError()!=MaterialImportPipelineError::None || receipt.assetId!="farm.material" || receipt.materialName!="farm" || receipt.contentHash==0U || materials.ResourceCount()!=1U) return 1;
    const AssetDefinition* definition=registry.Find("farm.material"); const CpuMaterialResource* resource=materials.Find("farm.material","farm");
    if (definition==nullptr || definition->kind!=AssetKind::Material || definition->state!=AssetState::Ready || resource==nullptr || resource->sourceHash!=receipt.contentHash || resource->material.rgba!=receipt.rgba) return 1;
    const size_t assetCount=registry.All().size(), materialCount=materials.ResourceCount(); const MaterialImportReceipt initial=receipt;
    const std::vector<uint8_t> replacement{'n','e','w','m','t','l',' ','f','a','r','m','\n','K','d',' ','0','.','8',' ','0','.','1',' ','0','.','2','\n','d',' ','0','.','7','5','\n'};
    if(!pipeline.RefreshMtl(registry,materials,"farm.material",replacement,"farm",receipt)||pipeline.LastError()!=MaterialImportPipelineError::None||receipt.contentHash==initial.contentHash||receipt.rgba==initial.rgba||!materials.IsCurrent(registry,"farm.material","farm"))return 1;
    const MaterialImportReceipt preserved=receipt; const CpuMaterialResource* refreshed=materials.Find("farm.material","farm"); if(refreshed==nullptr||refreshed->sourceHash!=receipt.contentHash)return 1;
    const std::vector<uint8_t> malformed{'n','e','w','m','t','l',' ','b','a','d','\n','K','d',' ','2',' ','0',' ','0','\n'};
    if (pipeline.ImportMtl(registry,materials,"bad.material",{},malformed,"bad",receipt) || pipeline.LastError()!=MaterialImportPipelineError::StageFailed || registry.All().size()!=assetCount || materials.ResourceCount()!=materialCount || receipt.assetId!=preserved.assetId || receipt.contentHash!=preserved.contentHash) return 1;
    if (pipeline.ImportMtl(registry,materials,"farm.material",{},valid,"farm",receipt) || pipeline.LastError()!=MaterialImportPipelineError::RegistryImportFailed || registry.All().size()!=assetCount || materials.ResourceCount()!=materialCount) return 1;
    if(pipeline.RefreshMtl(registry,materials,"farm.material",malformed,"farm",receipt)||pipeline.LastError()!=MaterialImportPipelineError::StageFailed||receipt.contentHash!=preserved.contentHash||!materials.IsCurrent(registry,"farm.material","farm"))return 1;
    if(pipeline.RefreshMtl(registry,materials,"missing.material",replacement,"farm",receipt)||pipeline.LastError()!=MaterialImportPipelineError::RegistryReplaceFailed||receipt.contentHash!=preserved.contentHash||!materials.IsCurrent(registry,"farm.material","farm"))return 1;
    if (pipeline.ImportMtl(registry,materials,"",{},valid,"farm",receipt) || pipeline.LastError()!=MaterialImportPipelineError::InvalidRequest || pipeline.ImportMtl(registry,materials,"empty.material",{},valid,"",receipt) || pipeline.LastError()!=MaterialImportPipelineError::InvalidRequest) return 1;
    std::printf("MATERIAL_IMPORT_PIPELINE_SMOKE_OK assets=%zu materials=%zu hash=%llu atomic=1\n", registry.All().size(), materials.ResourceCount(), static_cast<unsigned long long>(preserved.contentHash)); return 0;
}
