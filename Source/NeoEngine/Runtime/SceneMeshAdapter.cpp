#include "SceneMeshAdapter.h"

#include "MeshStaging.h"
#include "MaterialStaging.h"
#include "SoftwareRenderer.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {
namespace {
bool Finite(const Transform3& transform) { return std::isfinite(transform.x)&&std::isfinite(transform.y)&&std::isfinite(transform.z)&&std::isfinite(transform.rx)&&std::isfinite(transform.ry)&&std::isfinite(transform.rz)&&std::isfinite(transform.sx)&&std::isfinite(transform.sy)&&std::isfinite(transform.sz); }
bool CopyTexture(SceneMeshInstance& instance) {
    if(instance.material.texture==nullptr){instance.texture={};instance.sourceTextureAssetId.clear();instance.sourceTextureHash=0U;return true;}
    const CpuTextureResource& source=*instance.material.texture;
    if(source.assetId.empty()||source.sourceHash==0U||source.width==0U||source.height==0U||source.rgba.size()!=static_cast<size_t>(source.width)*source.height*4U)return false;
    instance.texture=source;instance.sourceTextureAssetId=source.assetId;instance.sourceTextureHash=source.sourceHash;instance.material.texture=&instance.texture;return true;
}
bool PrepareInstance(SceneMeshInstance& instance) {
    if(instance.entity.index==0xFFFFU||instance.vertices.empty()||instance.indices.empty()||instance.indices.size()%3U!=0U||instance.vertices.size()>MeshRenderer::kMaxVertices||instance.indices.size()>MeshRenderer::kMaxIndices)return false;
    float radius=0.0F;
    for(const MeshVertex& vertex:instance.vertices){if(!std::isfinite(vertex.position.x)||!std::isfinite(vertex.position.y)||!std::isfinite(vertex.position.z))return false;radius=std::max(radius,std::sqrt(vertex.position.x*vertex.position.x+vertex.position.y*vertex.position.y+vertex.position.z*vertex.position.z));}
    if(!std::isfinite(radius)||!CopyTexture(instance))return false;instance.localBoundsRadius=radius;return true;
}
}
bool SceneMeshAdapter::Add(SceneMeshInstance instance){if(instance.entity.index==0xFFFFU){lastError_=SceneMeshAdapterError::InvalidEntity;return false;}if(!PrepareInstance(instance)){lastError_=instance.material.texture==nullptr?SceneMeshAdapterError::InvalidMesh:SceneMeshAdapterError::InvalidTexture;return false;}if(std::any_of(instances_.begin(),instances_.end(),[&instance](const SceneMeshInstance& other){return other.entity==instance.entity;})){lastError_=SceneMeshAdapterError::InvalidEntity;return false;}if(instances_.size()>=kMaxInstances){lastError_=SceneMeshAdapterError::Capacity;return false;}instances_.push_back(std::move(instance));if(instances_.back().sourceTextureHash!=0U)instances_.back().material.texture=&instances_.back().texture;lastError_=SceneMeshAdapterError::None;return true;}
bool SceneMeshAdapter::AddStaged(SceneEntity entity,const CpuMeshResource& resource,MeshMaterial material){
    if(resource.assetId.empty()||resource.sourceHash==0U){lastError_=SceneMeshAdapterError::InvalidStagedResource;return false;}
    SceneMeshInstance instance{entity,resource.vertices,resource.indices,material};instance.sourceAssetId=resource.assetId;instance.sourceHash=resource.sourceHash;return Add(std::move(instance));
}
bool SceneMeshAdapter::AddStaged(SceneEntity entity,const CpuMeshResource& mesh,const CpuMaterialResource& material){
    if(material.assetId.empty()||material.materialName.empty()||material.sourceHash==0U){lastError_=SceneMeshAdapterError::InvalidStagedMaterial;return false;}
    if(mesh.assetId.empty()||mesh.sourceHash==0U){lastError_=SceneMeshAdapterError::InvalidStagedResource;return false;}
    SceneMeshInstance instance{entity,mesh.vertices,mesh.indices,material.material};instance.sourceAssetId=mesh.assetId;instance.sourceHash=mesh.sourceHash;instance.sourceMaterialAssetId=material.assetId;instance.sourceMaterialName=material.materialName;instance.sourceMaterialHash=material.sourceHash;return Add(std::move(instance));
}
bool SceneMeshAdapter::AddStaged(SceneEntity entity,const CpuMeshResource& mesh,const CpuMaterialResource& material,const CpuTextureResource* texture){
    if(mesh.assetId.empty()||mesh.sourceHash==0U){lastError_=SceneMeshAdapterError::InvalidStagedResource;return false;}
    if(material.assetId.empty()||material.materialName.empty()||material.sourceHash==0U){lastError_=SceneMeshAdapterError::InvalidStagedMaterial;return false;}
    MeshMaterial surface=material.material;surface.texture=texture;
    SceneMeshInstance instance{entity,mesh.vertices,mesh.indices,surface};instance.sourceAssetId=mesh.assetId;instance.sourceHash=mesh.sourceHash;instance.sourceMaterialAssetId=material.assetId;instance.sourceMaterialName=material.materialName;instance.sourceMaterialHash=material.sourceHash;return Add(std::move(instance));
}
bool SceneMeshAdapter::RefreshStaged(SceneEntity entity,const CpuMeshResource& resource,MeshMaterial material){
    if(resource.assetId.empty()||resource.sourceHash==0U){lastError_=SceneMeshAdapterError::InvalidStagedResource;return false;}
    const auto found=std::find_if(instances_.begin(),instances_.end(),[entity](const SceneMeshInstance& instance){return instance.entity==entity;});
    if(found==instances_.end()){lastError_=SceneMeshAdapterError::MissingInstance;return false;}
    if(found->sourceAssetId!=resource.assetId||!found->sourceMaterialAssetId.empty()){lastError_=SceneMeshAdapterError::InvalidStagedResource;return false;}
    const std::string textureId=material.texture==nullptr?std::string{}:material.texture->assetId;
    if(found->sourceTextureAssetId!=textureId){lastError_=SceneMeshAdapterError::InvalidStagedTexture;return false;}
    SceneMeshInstance replacement{entity,resource.vertices,resource.indices,material};replacement.sourceAssetId=resource.assetId;replacement.sourceHash=resource.sourceHash;
    if(!PrepareInstance(replacement)){lastError_=replacement.material.texture==nullptr?SceneMeshAdapterError::InvalidMesh:SceneMeshAdapterError::InvalidTexture;return false;}
    *found=std::move(replacement);if(found->sourceTextureHash!=0U)found->material.texture=&found->texture;lastError_=SceneMeshAdapterError::None;return true;
}
bool SceneMeshAdapter::CanRefreshStaged(SceneEntity entity,const CpuMeshResource& resource,MeshMaterial material) const {
    if(resource.assetId.empty()||resource.sourceHash==0U)return false;
    const auto found=std::find_if(instances_.begin(),instances_.end(),[entity](const SceneMeshInstance& instance){return instance.entity==entity;});
    if(found==instances_.end()||found->sourceAssetId!=resource.assetId||!found->sourceMaterialAssetId.empty())return false;
    const std::string textureId=material.texture==nullptr?std::string{}:material.texture->assetId;
    if(found->sourceTextureAssetId!=textureId)return false;
    SceneMeshInstance candidate{entity,resource.vertices,resource.indices,material};candidate.sourceAssetId=resource.assetId;candidate.sourceHash=resource.sourceHash;
    return PrepareInstance(candidate);
}
bool SceneMeshAdapter::RefreshStaged(SceneEntity entity,const CpuMeshResource& mesh,const CpuMaterialResource& material){
    if(mesh.assetId.empty()||mesh.sourceHash==0U){lastError_=SceneMeshAdapterError::InvalidStagedResource;return false;}
    if(material.assetId.empty()||material.materialName.empty()||material.sourceHash==0U){lastError_=SceneMeshAdapterError::InvalidStagedMaterial;return false;}
    const auto found=std::find_if(instances_.begin(),instances_.end(),[entity](const SceneMeshInstance& instance){return instance.entity==entity;});
    if(found==instances_.end()){lastError_=SceneMeshAdapterError::MissingInstance;return false;}
    if(found->sourceAssetId!=mesh.assetId){lastError_=SceneMeshAdapterError::InvalidStagedResource;return false;}
    if(found->sourceMaterialAssetId!=material.assetId||found->sourceMaterialName!=material.materialName){lastError_=SceneMeshAdapterError::InvalidStagedMaterial;return false;}
    if(!found->sourceTextureAssetId.empty()){lastError_=SceneMeshAdapterError::InvalidStagedTexture;return false;}
    SceneMeshInstance replacement{entity,mesh.vertices,mesh.indices,material.material};replacement.sourceAssetId=mesh.assetId;replacement.sourceHash=mesh.sourceHash;replacement.sourceMaterialAssetId=material.assetId;replacement.sourceMaterialName=material.materialName;replacement.sourceMaterialHash=material.sourceHash;
    if(!PrepareInstance(replacement)){lastError_=replacement.material.texture==nullptr?SceneMeshAdapterError::InvalidMesh:SceneMeshAdapterError::InvalidTexture;return false;}
    *found=std::move(replacement);if(found->sourceTextureHash!=0U)found->material.texture=&found->texture;lastError_=SceneMeshAdapterError::None;return true;
}
bool SceneMeshAdapter::CanRefreshStaged(SceneEntity entity,const CpuMeshResource& mesh,const CpuMaterialResource& material) const {
    if(mesh.assetId.empty()||mesh.sourceHash==0U||material.assetId.empty()||material.materialName.empty()||material.sourceHash==0U)return false;
    const auto found=std::find_if(instances_.begin(),instances_.end(),[entity](const SceneMeshInstance& instance){return instance.entity==entity;});
    if(found==instances_.end()||found->sourceAssetId!=mesh.assetId||found->sourceMaterialAssetId!=material.assetId||found->sourceMaterialName!=material.materialName||!found->sourceTextureAssetId.empty())return false;
    SceneMeshInstance candidate{entity,mesh.vertices,mesh.indices,material.material};candidate.sourceAssetId=mesh.assetId;candidate.sourceHash=mesh.sourceHash;candidate.sourceMaterialAssetId=material.assetId;candidate.sourceMaterialName=material.materialName;candidate.sourceMaterialHash=material.sourceHash;
    return PrepareInstance(candidate);
}
bool SceneMeshAdapter::Draw(const SceneWorld& world,RenderCamera& camera,SoftwareRenderer& renderer,const DirectionalLight& light){lastCulledCount_=0;MeshRenderer meshRenderer;for(const SceneMeshInstance& instance:instances_){const Transform3* transform=world.GetTransform(instance.entity);if(!transform){lastError_=SceneMeshAdapterError::MissingEntity;return false;}if(!Finite(*transform)||transform->sx<=0.0F||std::fabs(transform->sx-transform->sy)>0.0001F||std::fabs(transform->sx-transform->sz)>0.0001F){lastError_=SceneMeshAdapterError::UnsupportedTransform;return false;}const RenderPoint3 translation{transform->x,transform->y,transform->z};if(!camera.SphereIntersectsFrustum(translation,instance.localBoundsRadius*transform->sx)){++lastCulledCount_;continue;}if(!meshRenderer.Draw(instance.vertices,instance.indices,{translation,transform->sx,{transform->rx,transform->ry,transform->rz}},instance.material,light,camera,renderer)){lastError_=SceneMeshAdapterError::DrawFailed;return false;}}lastError_=SceneMeshAdapterError::None;return true;}
} // namespace NeoEngine
