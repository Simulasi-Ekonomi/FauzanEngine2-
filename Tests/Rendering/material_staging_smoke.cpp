#include "Runtime/MaterialStaging.h"

#include <cstdio>
#include <string>
#include <vector>

int main(){
    using namespace NeoEngine;
    const auto bytes=[](const std::string& value){return std::vector<uint8_t>(value.begin(),value.end());};const std::string mtl="newmtl grass\nKd 0.2 0.4 0.6\nd 0.5\n";
    AssetRegistry registry;if(!registry.ImportBytes("material.ready",AssetKind::Material,{},bytes(mtl))||!registry.MarkReady("material.ready")||!registry.ImportBytes("material.declared",AssetKind::Material,{},bytes(mtl))||!registry.ImportBytes("texture.wrong",AssetKind::Texture,{},bytes(mtl))||!registry.MarkReady("texture.wrong"))return 1;
    MaterialStagingStore staging;if(!staging.StageMtl(registry,"material.ready","grass")||staging.ResourceCount()!=1U)return 1;
    const CpuMaterialResource* resource=staging.Find("material.ready","grass");const AssetDefinition* definition=registry.Find("material.ready");if(resource==nullptr||definition==nullptr||!staging.IsCurrent(registry,"material.ready","grass")||resource->sourceHash!=definition->contentHash||resource->material.rgba!=0x80336699U||resource->material.ambient!=0.2F||resource->material.directional!=0.8F)return 1;const uint64_t expectedHash=definition->contentHash;
    if(staging.StageMtl(registry,"material.ready","grass")||staging.LastError()!=MaterialStagingError::DuplicateResource||staging.ResourceCount()!=1U)return 1;
    if(staging.StageMtl(registry,"material.declared","grass")||staging.LastError()!=MaterialStagingError::AssetNotReady||staging.ResourceCount()!=1U)return 1;
    if(staging.StageMtl(registry,"texture.wrong","grass")||staging.LastError()!=MaterialStagingError::WrongKind||staging.ResourceCount()!=1U)return 1;
    if(staging.StageMtl(registry,"material.ready","missing")||staging.LastError()!=MaterialStagingError::ImportFailed||staging.ResourceCount()!=1U)return 1;
    for(uint32_t index=0U;index<127U;++index){const std::string id="material.capacity."+std::to_string(index);if(!registry.ImportBytes(id,AssetKind::Material,{},bytes(mtl))||!registry.MarkReady(id)||!staging.StageMtl(registry,id,"grass"))return 1;}
    if(staging.ResourceCount()!=MaterialStagingStore::kMaxMaterials||resource->assetId!="material.ready"||resource->materialName!="grass"||resource->sourceHash!=expectedHash||resource->material.rgba!=0x80336699U)return 1;const std::string overflowId="material.capacity.overflow";if(!registry.ImportBytes(overflowId,AssetKind::Material,{},bytes(mtl))||!registry.MarkReady(overflowId)||staging.StageMtl(registry,overflowId,"grass")||staging.LastError()!=MaterialStagingError::CapacityExceeded||staging.ResourceCount()!=MaterialStagingStore::kMaxMaterials||!registry.ReplaceBytes("material.ready",bytes("newmtl grass\nKd 0.8 0.2 0.1\nd 1\n"))||staging.IsCurrent(registry,"material.ready","grass"))return 1;
    const uint64_t refreshedHash=resource->sourceHash;if(!staging.CanRefresh(registry,"material.ready","grass")||!staging.Refresh(registry,"material.ready","grass")||staging.Find("material.ready","grass")!=resource||!staging.IsCurrent(registry,"material.ready","grass")||resource->sourceHash==refreshedHash||resource->material.rgba==0x80336699U||staging.ResourceCount()!=MaterialStagingStore::kMaxMaterials)return 1;
    const uint64_t hashAfterRefresh=resource->sourceHash;const uint32_t rgbaAfterRefresh=resource->material.rgba;if(!registry.ReplaceBytes("material.ready",bytes("newmtl other\nKd 0.1 0.2 0.3\nd 1\n"))||staging.IsCurrent(registry,"material.ready","grass")||staging.CanRefresh(registry,"material.ready","grass")||staging.Refresh(registry,"material.ready","grass")||staging.LastError()!=MaterialStagingError::ImportFailed||staging.Find("material.ready","grass")!=resource||resource->sourceHash!=hashAfterRefresh||resource->material.rgba!=rgbaAfterRefresh||staging.ResourceCount()!=MaterialStagingStore::kMaxMaterials||staging.IsCurrent(registry,"material.ready","grass"))return 1;
    std::printf("MATERIAL_STAGING_SMOKE_OK resources=%zu hash=1 rgba=%08X staleDetection=1 refresh=1 probe=1 stablePointer=1 atomic=1\n",staging.ResourceCount(),resource->material.rgba);
}
