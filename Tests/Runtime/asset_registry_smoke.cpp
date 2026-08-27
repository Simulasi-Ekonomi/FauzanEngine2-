#include "Runtime/AssetRegistry.h"
#include <cstdio>

using namespace NeoEngine;

int main(){
    AssetRegistry r;
    bool ok=r.ImportBytes("farm-tile",AssetKind::Texture,{}, {1,2,3})&&r.Declare("farm-mat",AssetKind::Material,{"farm-tile"})&&!r.MarkReady("farm-mat")&&r.MarkReady("farm-tile")&&r.MarkReady("farm-mat")&&!r.Declare("farm-mat",AssetKind::Material,{})&&!r.Declare("bad/id",AssetKind::Prefab,{})&&r.Find("farm-tile")->contentHash!=0&&r.Data("farm-tile")->size()==3;
    const AssetRegistrySummary ready=r.Summary();
    ok=ok&&ready.assetCount==2U&&ready.readyAssetCount==2U&&ready.storedByteCount==3U;
    if(!ok){std::fprintf(stderr,"ASSET_REGISTRY_SMOKE_FAIL\n");return 1;}
    const AssetDefinition* tile=r.Find("farm-tile");
    const std::vector<uint8_t>* tileBytes=r.Data("farm-tile");
    if(!tile||!tileBytes)return 1;
    const uint64_t originalHash=tile->contentHash;
    if(!r.ReplaceBytes("farm-tile",{9,8})||tile->contentHash==originalHash||tile->byteSize!=2U||tileBytes->size()!=2U||(*tileBytes)[0U]!=9U||(*tileBytes)[1U]!=8U)return 1;
    const uint64_t replacementHash=tile->contentHash;
    if(r.ReplaceBytes("missing",{1U})||r.LastError()!=AssetRegistryError::MissingAsset||r.ReplaceBytes("farm-tile",std::vector<uint8_t>(AssetRegistry::kMaxAssetBytes+1U,1U))||r.LastError()!=AssetRegistryError::ByteLimitExceeded||tile->contentHash!=replacementHash||tileBytes->size()!=2U||(*tileBytes)[0U]!=9U)return 1;
    for(uint32_t index=0;index<512U;++index){if(!r.ImportBytes("asset"+std::to_string(index),AssetKind::Audio,{}, {static_cast<uint8_t>(index)}))return 1;}
    const AssetRegistrySummary final=r.Summary();
    if(tile->id!="farm-tile"||tile->contentHash!=replacementHash||tileBytes->size()!=2U||(*tileBytes)[0U]!=9U||(*tileBytes)[1U]!=8U||final.assetCount!=514U||final.readyAssetCount!=2U||final.storedByteCount!=514U){std::fprintf(stderr,"ASSET_REGISTRY_POINTER_STABILITY_FAIL\n");return 1;}
    std::printf("ASSET_REGISTRY_SMOKE_OK assets=%u replacement=1 summary=1 deterministic=1 stablePointer=1\n",final.assetCount);
}
