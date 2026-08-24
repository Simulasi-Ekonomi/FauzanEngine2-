#include "Runtime/MeshStaging.h"

#include <cstdio>
#include <string>
#include <vector>

int main(){
    using namespace NeoEngine;
    const auto bytes=[](const std::string& value){return std::vector<uint8_t>(value.begin(),value.end());};
    const std::string quad="v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 3//1 4//1\n";
    AssetRegistry registry;if(!registry.ImportBytes("mesh.ready",AssetKind::Mesh,{},bytes(quad))||!registry.MarkReady("mesh.ready"))return 1;
    const std::string flat="v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    if(!registry.ImportBytes("mesh.declared",AssetKind::Mesh,{},bytes(quad))||!registry.ImportBytes("texture.wrong",AssetKind::Texture,{},bytes(quad))||!registry.MarkReady("texture.wrong")||!registry.ImportBytes("mesh.flat",AssetKind::Mesh,{},bytes(flat))||!registry.MarkReady("mesh.flat")||!registry.ImportBytes("mesh.bad",AssetKind::Mesh,{},bytes("v 0 0 0\nf 1 2 3\n"))||!registry.MarkReady("mesh.bad"))return 1;
    MeshStagingStore staging;if(!staging.StageObj(registry,"mesh.ready")||staging.ResourceCount()!=1U||staging.StagedVertices()!=4U||staging.StagedIndices()!=6U)return 1;
    const CpuMeshResource* resource=staging.Find("mesh.ready");const AssetDefinition* definition=registry.Find("mesh.ready");if(resource==nullptr||definition==nullptr||!staging.IsCurrent(registry,"mesh.ready")||resource->sourceHash!=definition->contentHash)return 1;
    if(staging.StageObj(registry,"mesh.ready")||staging.LastError()!=MeshStagingError::DuplicateResource||staging.ResourceCount()!=1U)return 1;
    if(staging.StageObj(registry,"mesh.declared")||staging.LastError()!=MeshStagingError::AssetNotReady||staging.ResourceCount()!=1U)return 1;
    if(staging.StageObj(registry,"texture.wrong")||staging.LastError()!=MeshStagingError::WrongKind||staging.ResourceCount()!=1U)return 1;
    if(staging.StageObj(registry,"mesh.flat")||staging.LastError()!=MeshStagingError::ImportFailed||staging.ResourceCount()!=1U)return 1;
    if(!staging.StageObj(registry,"mesh.flat",{true})||staging.ResourceCount()!=2U||staging.StagedVertices()!=7U||staging.StagedIndices()!=9U||resource->assetId!="mesh.ready"||resource->vertices.size()!=4U||resource->sourceHash!=definition->contentHash)return 1;
    const CpuMeshResource* flatResource=staging.Find("mesh.flat");const AssetDefinition* flatDefinition=registry.Find("mesh.flat");if(flatResource==nullptr||flatDefinition==nullptr||!flatResource->generatedFlatNormals||flatResource->sourceHash!=flatDefinition->contentHash||flatResource->vertices.size()!=3U||flatResource->vertices[0].normal.z!=1.0F)return 1;
    if(staging.StageObj(registry,"mesh.bad")||staging.LastError()!=MeshStagingError::ImportFailed||staging.ResourceCount()!=2U)return 1;
    std::string nearCapacity;for(uint32_t vertex=0U;vertex<2046U;++vertex)nearCapacity+="v "+std::to_string(vertex)+" 0 0\n";nearCapacity+="vn 0 0 1\n";for(uint32_t vertex=1U;vertex<=2046U;vertex+=3U)nearCapacity+="f "+std::to_string(vertex)+"//1 "+std::to_string(vertex+1U)+"//1 "+std::to_string(vertex+2U)+"//1\n";
    if(!registry.ImportBytes("mesh.capacity",AssetKind::Mesh,{},bytes(nearCapacity))||!registry.MarkReady("mesh.capacity")||staging.StageObj(registry,"mesh.capacity")||staging.LastError()!=MeshStagingError::CapacityExceeded||staging.ResourceCount()!=2U||staging.StagedVertices()!=7U||staging.StagedIndices()!=9U||!registry.ReplaceBytes("mesh.ready",bytes(quad+"# replacement\n"))||staging.IsCurrent(registry,"mesh.ready"))return 1;
    const uint64_t refreshedReadyHash=resource->sourceHash;if(!staging.CanRefresh(registry,"mesh.ready")||!staging.Refresh(registry,"mesh.ready")||staging.Find("mesh.ready")!=resource||!staging.IsCurrent(registry,"mesh.ready")||resource->sourceHash==refreshedReadyHash||resource->vertices.size()!=4U||staging.StagedVertices()!=7U||staging.StagedIndices()!=9U)return 1;
    const std::string flatReplacement="v 0 0 0\nv 2 0 0\nv 0 2 0\nf 1 2 3\n";const uint64_t flatHash=flatResource->sourceHash;if(!registry.ReplaceBytes("mesh.flat",bytes(flatReplacement))||staging.IsCurrent(registry,"mesh.flat")||!staging.CanRefresh(registry,"mesh.flat")||!staging.Refresh(registry,"mesh.flat")||staging.Find("mesh.flat")!=flatResource||!staging.IsCurrent(registry,"mesh.flat")||flatResource->sourceHash==flatHash||!flatResource->generatedFlatNormals||flatResource->vertices[0].normal.z!=1.0F||staging.StagedVertices()!=7U||staging.StagedIndices()!=9U)return 1;
    const uint64_t readyHashAfterRefresh=resource->sourceHash;if(!registry.ReplaceBytes("mesh.ready",bytes("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n"))||staging.IsCurrent(registry,"mesh.ready")||staging.CanRefresh(registry,"mesh.ready")||staging.Refresh(registry,"mesh.ready")||staging.LastError()!=MeshStagingError::ImportFailed||staging.Find("mesh.ready")!=resource||resource->sourceHash!=readyHashAfterRefresh||resource->vertices.size()!=4U||staging.StagedVertices()!=7U||staging.StagedIndices()!=9U||staging.IsCurrent(registry,"mesh.ready"))return 1;
    std::printf("MESH_STAGING_SMOKE_OK resources=2 vertices=%zu indices=%zu hash=1 flatNormal=1 staleDetection=1 refresh=1 probe=1 stablePointer=1 atomic=1\n",staging.StagedVertices(),staging.StagedIndices());
}
