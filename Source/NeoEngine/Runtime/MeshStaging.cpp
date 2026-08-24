#include "MeshStaging.h"

#include <algorithm>

namespace NeoEngine {
bool MeshStagingStore::StageObj(const AssetRegistry& registry,std::string_view assetId,const MeshStagingOptions& options){
    const AssetDefinition* definition=registry.Find(assetId);const std::vector<uint8_t>* bytes=registry.Data(assetId);
    if(definition==nullptr||bytes==nullptr){lastError_=MeshStagingError::MissingAsset;return false;}
    if(definition->kind!=AssetKind::Mesh){lastError_=MeshStagingError::WrongKind;return false;}
    if(definition->state!=AssetState::Ready){lastError_=MeshStagingError::AssetNotReady;return false;}
    if(Find(assetId)!=nullptr){lastError_=MeshStagingError::DuplicateResource;return false;}
    if(resources_.size()>=kMaxMeshes){lastError_=MeshStagingError::CapacityExceeded;return false;}
    ObjMeshImporter importer;std::vector<MeshVertex> vertices;std::vector<uint16_t> indices;const std::string_view source{reinterpret_cast<const char*>(bytes->data()),bytes->size()};
    if(!importer.Import(source,vertices,indices,{options.generateFlatNormals})){lastError_=MeshStagingError::ImportFailed;return false;}
    if(vertices.size()>kMaxStoredVertices-stagedVertices_||indices.size()>kMaxStoredIndices-stagedIndices_){lastError_=MeshStagingError::CapacityExceeded;return false;}
    stagedVertices_+=vertices.size();stagedIndices_+=indices.size();resources_.push_back({std::string(assetId),definition->contentHash,options.generateFlatNormals,std::move(vertices),std::move(indices)});lastError_=MeshStagingError::None;return true;
}
bool MeshStagingStore::Refresh(const AssetRegistry& registry,std::string_view assetId){
    const AssetDefinition* definition=registry.Find(assetId);const std::vector<uint8_t>* bytes=registry.Data(assetId);
    if(definition==nullptr||bytes==nullptr){lastError_=MeshStagingError::MissingAsset;return false;}
    if(definition->kind!=AssetKind::Mesh){lastError_=MeshStagingError::WrongKind;return false;}
    if(definition->state!=AssetState::Ready){lastError_=MeshStagingError::AssetNotReady;return false;}
    const auto found=std::find_if(resources_.begin(),resources_.end(),[assetId](const CpuMeshResource& resource){return resource.assetId==assetId;});
    if(found==resources_.end()){lastError_=MeshStagingError::MissingAsset;return false;}
    ObjMeshImporter importer;std::vector<MeshVertex> vertices;std::vector<uint16_t> indices;const std::string_view source{reinterpret_cast<const char*>(bytes->data()),bytes->size()};
    if(!importer.Import(source,vertices,indices,{found->generatedFlatNormals})){lastError_=MeshStagingError::ImportFailed;return false;}
    const size_t retainedVertices=stagedVertices_-found->vertices.size();const size_t retainedIndices=stagedIndices_-found->indices.size();
    if(vertices.size()>kMaxStoredVertices-retainedVertices||indices.size()>kMaxStoredIndices-retainedIndices){lastError_=MeshStagingError::CapacityExceeded;return false;}
    const size_t replacementVertices=vertices.size();const size_t replacementIndices=indices.size();
    found->sourceHash=definition->contentHash;found->vertices=std::move(vertices);found->indices=std::move(indices);
    stagedVertices_=retainedVertices+replacementVertices;stagedIndices_=retainedIndices+replacementIndices;lastError_=MeshStagingError::None;return true;
}
bool MeshStagingStore::CanRefresh(const AssetRegistry& registry,std::string_view assetId) const {
    const AssetDefinition* definition=registry.Find(assetId);const std::vector<uint8_t>* bytes=registry.Data(assetId);
    if(definition==nullptr||bytes==nullptr||definition->kind!=AssetKind::Mesh||definition->state!=AssetState::Ready)return false;
    const CpuMeshResource* found=Find(assetId);if(found==nullptr)return false;
    ObjMeshImporter importer;std::vector<MeshVertex> vertices;std::vector<uint16_t> indices;const std::string_view source{reinterpret_cast<const char*>(bytes->data()),bytes->size()};
    if(!importer.Import(source,vertices,indices,{found->generatedFlatNormals}))return false;
    const size_t retainedVertices=stagedVertices_-found->vertices.size();const size_t retainedIndices=stagedIndices_-found->indices.size();
    return vertices.size()<=kMaxStoredVertices-retainedVertices&&indices.size()<=kMaxStoredIndices-retainedIndices;
}
const CpuMeshResource* MeshStagingStore::Find(std::string_view assetId) const {
    const auto found=std::find_if(resources_.begin(),resources_.end(),[assetId](const CpuMeshResource& resource){return resource.assetId==assetId;});return found==resources_.end()?nullptr:&*found;
}
} // namespace NeoEngine
