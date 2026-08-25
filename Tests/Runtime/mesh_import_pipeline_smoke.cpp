#include "Runtime/MeshImportPipeline.h"

#include <cstdio>
#include <string>
#include <vector>

int main() {
    using namespace NeoEngine;
    const auto bytes=[](const std::string& value){return std::vector<uint8_t>(value.begin(),value.end());}; const std::string quad="v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 3//1 4//1\n";
    AssetRegistry registry; MeshStagingStore meshes; MeshImportPipeline pipeline; MeshImportReceipt receipt{};
    if(!pipeline.ImportObj(registry,meshes,"pipeline.obj",{},bytes(quad),{},receipt)||pipeline.LastError()!=MeshImportPipelineError::None||receipt.vertexCount!=4U||receipt.indexCount!=6U||receipt.generatedFlatNormals||registry.Find("pipeline.obj")==nullptr||registry.Find("pipeline.obj")->state!=AssetState::Ready||meshes.Find("pipeline.obj")==nullptr)return 1;
    const MeshImportReceipt preserved=receipt; const size_t assets=registry.All().size(), staged=meshes.ResourceCount(); const std::string invalid="v 0 0 0\nf 1 2 3\n";
    if(pipeline.ImportObj(registry,meshes,"broken.obj",{},bytes(invalid),{},receipt)||pipeline.LastError()!=MeshImportPipelineError::StageFailed||registry.All().size()!=assets||meshes.ResourceCount()!=staged||receipt.assetId!=preserved.assetId)return 1;
    if(pipeline.ImportObj(registry,meshes,"pipeline.obj",{},bytes(quad),{},receipt)||pipeline.LastError()!=MeshImportPipelineError::RegistryImportFailed||registry.All().size()!=assets||meshes.ResourceCount()!=staged||receipt.assetId!=preserved.assetId)return 1;
    if(pipeline.ImportObj(registry,meshes,"",{},bytes(quad),{},receipt)||pipeline.LastError()!=MeshImportPipelineError::InvalidRequest||registry.All().size()!=assets||meshes.ResourceCount()!=staged)return 1;
    std::printf("MESH_IMPORT_PIPELINE_SMOKE_OK registry=1 ready=1 staged=1 atomic=1 hash=%llu\n",static_cast<unsigned long long>(receipt.contentHash)); return 0;
}
