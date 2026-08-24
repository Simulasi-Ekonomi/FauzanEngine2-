#include "MaterialStaging.h"

#include <algorithm>

namespace NeoEngine {
bool MaterialStagingStore::StageMtl(const AssetRegistry& registry,std::string_view assetId,std::string_view materialName){
    const AssetDefinition* definition=registry.Find(assetId);const std::vector<uint8_t>* bytes=registry.Data(assetId);
    if(definition==nullptr||bytes==nullptr){lastError_=MaterialStagingError::MissingAsset;return false;}
    if(definition->kind!=AssetKind::Material){lastError_=MaterialStagingError::WrongKind;return false;}
    if(definition->state!=AssetState::Ready){lastError_=MaterialStagingError::AssetNotReady;return false;}
    if(Find(assetId,materialName)!=nullptr){lastError_=MaterialStagingError::DuplicateResource;return false;}
    if(resources_.size()>=kMaxMaterials){lastError_=MaterialStagingError::CapacityExceeded;return false;}
    MtlMaterialImporter importer;MeshMaterial material{0xFFFFFFFFU,0.2F,0.8F,nullptr};const std::string_view source{reinterpret_cast<const char*>(bytes->data()),bytes->size()};
    if(!importer.Import(source,materialName,material)){lastError_=MaterialStagingError::ImportFailed;return false;}
    resources_.push_back({std::string(assetId),std::string(materialName),definition->contentHash,material});lastError_=MaterialStagingError::None;return true;
}
bool MaterialStagingStore::Refresh(const AssetRegistry& registry,std::string_view assetId,std::string_view materialName){
    const AssetDefinition* definition=registry.Find(assetId);const std::vector<uint8_t>* bytes=registry.Data(assetId);
    if(definition==nullptr||bytes==nullptr){lastError_=MaterialStagingError::MissingAsset;return false;}
    if(definition->kind!=AssetKind::Material){lastError_=MaterialStagingError::WrongKind;return false;}
    if(definition->state!=AssetState::Ready){lastError_=MaterialStagingError::AssetNotReady;return false;}
    const auto found=std::find_if(resources_.begin(),resources_.end(),[assetId,materialName](const CpuMaterialResource& resource){return resource.assetId==assetId&&resource.materialName==materialName;});
    if(found==resources_.end()){lastError_=MaterialStagingError::MissingAsset;return false;}
    MtlMaterialImporter importer;MeshMaterial material{0xFFFFFFFFU,0.2F,0.8F,nullptr};const std::string_view source{reinterpret_cast<const char*>(bytes->data()),bytes->size()};
    if(!importer.Import(source,materialName,material)){lastError_=MaterialStagingError::ImportFailed;return false;}
    found->sourceHash=definition->contentHash;found->material=material;lastError_=MaterialStagingError::None;return true;
}
bool MaterialStagingStore::CanRefresh(const AssetRegistry& registry,std::string_view assetId,std::string_view materialName) const {
    const AssetDefinition* definition=registry.Find(assetId);const std::vector<uint8_t>* bytes=registry.Data(assetId);
    if(definition==nullptr||bytes==nullptr||definition->kind!=AssetKind::Material||definition->state!=AssetState::Ready||Find(assetId,materialName)==nullptr)return false;
    MtlMaterialImporter importer;MeshMaterial material{0xFFFFFFFFU,0.2F,0.8F,nullptr};const std::string_view source{reinterpret_cast<const char*>(bytes->data()),bytes->size()};
    return importer.Import(source,materialName,material);
}
const CpuMaterialResource* MaterialStagingStore::Find(std::string_view assetId,std::string_view materialName) const {
    const auto found=std::find_if(resources_.begin(),resources_.end(),[assetId,materialName](const CpuMaterialResource& resource){return resource.assetId==assetId&&resource.materialName==materialName;});return found==resources_.end()?nullptr:&*found;
}
} // namespace NeoEngine
