#pragma once

#include "MeshRenderer.h"
#include "SceneWorld.h"
#include "TextureStaging.h"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace NeoEngine {
struct CpuMeshResource;
struct CpuMaterialResource;
enum class SceneMeshAdapterError : uint8_t { None, InvalidEntity, InvalidMesh, InvalidTexture, InvalidStagedResource, InvalidStagedMaterial, InvalidStagedTexture, Capacity, MissingInstance, MissingEntity, UnsupportedTransform, DrawFailed };
struct SceneMeshInstance { SceneEntity entity{}; std::vector<MeshVertex> vertices; std::vector<uint16_t> indices; MeshMaterial material{}; float localBoundsRadius = 0.0F; std::string sourceAssetId{}; uint64_t sourceHash = 0U; std::string sourceMaterialAssetId{}; std::string sourceMaterialName{}; uint64_t sourceMaterialHash = 0U; CpuTextureResource texture{}; std::string sourceTextureAssetId{}; uint64_t sourceTextureHash = 0U; };
class SceneMeshAdapter {
public:
    static constexpr uint16_t kMaxInstances = 64;
    SceneMeshAdapter() = default;
    SceneMeshAdapter(const SceneMeshAdapter& other);
    SceneMeshAdapter& operator=(const SceneMeshAdapter& other);
    SceneMeshAdapter(SceneMeshAdapter&& other);
    SceneMeshAdapter& operator=(SceneMeshAdapter&& other);
    bool Add(SceneMeshInstance instance);
    bool AddStaged(SceneEntity entity,const CpuMeshResource& resource,MeshMaterial material);
    bool AddStaged(SceneEntity entity,const CpuMeshResource& mesh,const CpuMaterialResource& material);
    bool AddStaged(SceneEntity entity,const CpuMeshResource& mesh,const CpuMaterialResource& material,const CpuTextureResource* texture);
    // Explicitly replaces a copy-on-register CPU instance only when its staged source identity matches.
    bool RefreshStaged(SceneEntity entity,const CpuMeshResource& resource,MeshMaterial material);
    bool RefreshStaged(SceneEntity entity,const CpuMeshResource& mesh,const CpuMaterialResource& material);
    // Validates the same source identity and candidate CPU instance shape as RefreshStaged without replacement.
    [[nodiscard]] bool CanRefreshStaged(SceneEntity entity,const CpuMeshResource& resource,MeshMaterial material) const;
    [[nodiscard]] bool CanRefreshStaged(SceneEntity entity,const CpuMeshResource& mesh,const CpuMaterialResource& material) const;
    bool Draw(const SceneWorld& world, RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light);
    [[nodiscard]] const std::deque<SceneMeshInstance>& Instances() const { return instances_; }
    [[nodiscard]] SceneMeshAdapterError LastError() const { return lastError_; }
    [[nodiscard]] uint16_t LastCulledCount() const { return lastCulledCount_; }
private:
    void RebindEmbeddedTexturePointers();
    std::deque<SceneMeshInstance> instances_;
    SceneMeshAdapterError lastError_ = SceneMeshAdapterError::None;
    uint16_t lastCulledCount_ = 0;
};
} // namespace NeoEngine
