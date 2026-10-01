#include "Runtime/NeoRuntime.h"
#include "Runtime/MeshStaging.h"
#include "Runtime/MaterialStaging.h"
#include "Animation/Bone.h"
#include "Animation/SkeletalPoseClip.h"
#include "Animation/Skeleton.h"
#include <cstdio>
#include <cmath>
#include <limits>
int main() {
    NeoEngine::NeoRuntime runtime;
    NeoEngine::RuntimeConfig config{};
    config.farmNpcCount = 1;
    config.renderWidth = 64;
    config.renderHeight = 48;
    if (!runtime.Initialize(config)) return 1;
    if (runtime.ECS() == nullptr || runtime.Scene() == nullptr || runtime.SceneMeshes() == nullptr) return 2;
    if (runtime.Scene()->AliveCount() > NeoEngine::SceneWorld::kCapacity) return 3;
    if (runtime.SceneECS().sceneCount != runtime.Scene()->AliveCount()) return 5;
    if (runtime.SceneECS().ecsCount != runtime.Scene()->AliveCount()) return 21;
    const auto entities = runtime.Scene()->AliveEntities();
    if (entities.size() != runtime.Scene()->AliveCount()) return 4;
    assert(!entities.empty());
    const NeoEngine::EntityID ecsId = runtime.SceneECS().ecsCount == 0U ? std::numeric_limits<NeoEngine::EntityID>::max() : 0U;
    if (ecsId == std::numeric_limits<NeoEngine::EntityID>::max() || !runtime.ECS()->HasEntity(ecsId)) return 6;
    float x=0.0F,y=0.0F,z=0.0F,rx=0.0F,ry=0.0F,rz=0.0F,sx=0.0F,sy=0.0F,sz=0.0F;
    if (!runtime.ECS()->TryGetPosition(ecsId,x,y,z)) return 7;
    if (!runtime.ECS()->TryGetRotation(ecsId,rx,ry,rz)) return 8;
    if (!runtime.ECS()->TryGetScale(ecsId,sx,sy,sz)) return 9;
    if (!(sx==1.0F && sy==1.0F && sz==1.0F)) return 10;
    if (!(std::isfinite(x) && std::isfinite(y) && std::isfinite(z))) return 11;
    if (!(std::isfinite(rx) && std::isfinite(ry) && std::isfinite(rz))) return 12;

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
    if (!runtime.Tick()) return 17;
    const NeoEngine::NeoRuntimeFrameReceipt* receipt = runtime.LastFrameReceipt();
    if (receipt == nullptr) return 18;
    assert(receipt->frameStage == NeoEngine::RuntimeFrameStage::Completed);
    assert(receipt->frameToken.frame == receipt->clock.frameCount);
    assert(receipt->frameToken.revision <= receipt->sceneECS.revision);
    assert(!receipt->hasVulkanRenderReceipt);
    if (runtime.SceneECS().sceneCount != runtime.Scene()->AliveCount()) return 20;
    if (runtime.SceneECS().ecsCount != runtime.Scene()->AliveCount()) return 31;
    const NeoEngine::EntityID meshEcsId = ecsId;
    if (meshEcsId == std::numeric_limits<NeoEngine::EntityID>::max()) return 22;
    if ((runtime.ECS()->GetComponentMask(meshEcsId) & NeoEngine::COMP_MESH) == 0U) return 23;
    uint64_t meshHash=0U, materialHash=0U;
    if (!runtime.ECS()->TryGetMeshAssetIdentity(meshEcsId, meshHash, materialHash)) return 24;
    if (meshHash != mesh.sourceHash || materialHash != material.sourceHash) return 25;
    if (!runtime.SetPaused(true)) return 26;
    if (!runtime.Tick()) return 27;
    receipt = runtime.LastFrameReceipt();
    if (receipt == nullptr || receipt->clock.frameCount == 0U) return 28;
    if (!runtime.SetPaused(false)) return 29;
    if (runtime.SceneECS().sceneCount != runtime.Scene()->AliveCount()) return 30;
    assert(runtime.SceneECS().ecsCount == runtime.Scene()->AliveCount());
    if (!runtime.Shutdown()) return 32;
    return 0;
}
