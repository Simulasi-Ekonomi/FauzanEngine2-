#include "Runtime/NeoRuntime.h"
#include "Runtime/MeshStaging.h"
#include "Runtime/MaterialStaging.h"
#include "Animation/Bone.h"
#include "Animation/SkeletalPoseClip.h"
#include "Animation/Skeleton.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <limits>
int main() {
    NeoEngine::NeoRuntime runtime;
    NeoEngine::RuntimeConfig config{};
    config.farmNpcCount = 1;
    config.renderWidth = 64;
    config.renderHeight = 48;
    assert(runtime.Initialize(config));
    assert(runtime.ECS() != nullptr);
    assert(runtime.Scene() != nullptr);
    assert(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    assert(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    const auto entities = runtime.Scene()->AliveEntities();
    assert(!entities.empty());
    const NeoEngine::EntityID ecsId = runtime.SceneECSId(entities.front());
    assert(ecsId != std::numeric_limits<NeoEngine::EntityID>::max());
    float x=0.0F,y=0.0F,z=0.0F,rx=0.0F,ry=0.0F,rz=0.0F,sx=0.0F,sy=0.0F,sz=0.0F;
    assert(runtime.ECS()->TryGetPosition(ecsId,x,y,z));
    assert(runtime.ECS()->TryGetRotation(ecsId,rx,ry,rz));
    assert(runtime.ECS()->TryGetScale(ecsId,sx,sy,sz));
    assert(sx==1.0F && sy==1.0F && sz==1.0F);
    assert(std::isfinite(x) && std::isfinite(y) && std::isfinite(z));
    assert(std::isfinite(rx) && std::isfinite(ry) && std::isfinite(rz));

    NeoEngine::CpuMeshResource mesh{};
    mesh.assetId = "smoke.mesh";
    mesh.sourceHash = 0x1020304050607080ULL;
    mesh.vertices = {
        {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        {{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, 1.0F, 0.0F},
        {{0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, 0.0F, 1.0F}
    };
    mesh.indices = {0U, 1U, 2U};
    NeoEngine::CpuMaterialResource material{};
    material.assetId = "smoke.material";
    material.materialName = "default";
    material.sourceHash = 0x9080706050403020ULL;
    assert(runtime.SceneMeshes()->AddStaged(entities.front(), mesh, material));

    NeoEngine::Skeleton skeleton;
    NeoEngine::Bone root("root", -1);
    if (!skeleton.TryAddBone(root) || !skeleton.DeriveInverseBindPose()) return 12;
    NeoEngine::SkeletalPoseClip clip;
    if (!clip.Configure(1U)) return 13;
    const std::vector<NeoEngine::SkeletalPoseKeyframe> keys{
        {0.0F, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 1.0F}, {1.0F, 1.0F, 1.0F}},
        {0.5F, {0.1F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 1.0F}, {1.0F, 1.0F, 1.0F}}
    };
    if (!clip.SetTrack(0U, keys)) return 14;
    std::vector<NeoEngine::VertexWeight> weights(mesh.vertices.size());
    for (auto& weight : weights) { weight.boneIDs[0] = 0; weight.weights[0] = 1.0F; }
    if (!runtime.SceneMeshes()->BindSkeletalAnimation(entities.front(), skeleton, clip, NeoEngine::SkeletalPosePlaybackMode::Clamp, weights)) return 15;
    if (runtime.SceneMeshes()->Instances().front().skeletalPalette.size() != 1U) return 16;
    assert(runtime.Tick());
    const NeoEngine::NeoRuntimeFrameReceipt* receipt = runtime.LastFrameReceipt();
    assert(receipt != nullptr);
    assert(receipt->frameStage == NeoEngine::RuntimeFrameStage::Completed);
    assert(receipt->frameToken.frame == receipt->clock.frameCount);
    assert(receipt->frameToken.revision <= receipt->sceneECS.revision);
    assert(!receipt->hasVulkanRenderReceipt);
    assert(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    assert(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    const NeoEngine::EntityID meshEcsId = runtime.SceneECSId(entities.front());
    assert(meshEcsId != std::numeric_limits<NeoEngine::EntityID>::max());
    assert((runtime.ECS()->GetComponentMask(meshEcsId) & NeoEngine::COMP_MESH) != 0U);
    uint64_t meshHash=0U, materialHash=0U;
    assert(runtime.ECS()->TryGetMeshAssetIdentity(meshEcsId, meshHash, materialHash));
    assert(meshHash == mesh.sourceHash && materialHash == material.sourceHash);
    assert(runtime.SetPaused(true));
    assert(runtime.Tick());
    receipt = runtime.LastFrameReceipt();
    assert(receipt != nullptr && receipt->frameStage == NeoEngine::RuntimeFrameStage::Completed);
    assert(runtime.SetPaused(false));
    assert(runtime.SceneECS().sceneCount == runtime.Scene()->AliveCount());
    assert(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    assert(runtime.Shutdown());
    return 0;
}
