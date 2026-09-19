#include "Runtime/SceneMeshAdapter.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
NeoEngine::Mat4 Translation(float x, float y, float z) {
    NeoEngine::Mat4 m{};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0F;
    m.m[12] = x; m.m[13] = y; m.m[14] = z;
    return m;
}
NeoEngine::SkeletalPoseKeyframe Key(float time, float x, float y, float z) {
    NeoEngine::SkeletalPoseKeyframe k{};
    k.time = time; k.translation = {x, y, z};
    return k;
}
bool Near(float a, float b) { return std::fabs(a - b) < 0.0001F; }
}

int main() {
    using namespace NeoEngine;
    SceneMeshAdapter adapter;
    SceneEntity entity{};
    std::vector<MeshVertex> vertices(1);
    vertices[0].position = {0.0F, 0.0F, 0.0F};
    std::vector<uint16_t> indices{0, 0, 0};
    MeshMaterial material{};
    if (!adapter.Add({entity, vertices, indices, material})) return 1;

    Skeleton skeleton;
    Bone root{"root", -1};
    root.localBindPose = Translation(0.0F, 0.0F, 0.0F);
    if (!skeleton.TryAddBone(root)) return 1;

    SkeletalPoseClip clip;
    if (!clip.Configure(1U) ||
        !clip.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{
            Key(0.0F, 0.0F, 0.0F, 0.0F),
            Key(1.0F, 2.0F, 0.0F, 0.0F)})) return 1;

    VertexWeight weight{};
    weight.boneIDs[0] = 0;
    weight.weights[0] = 1.0F;
    if (!adapter.BindSkeletalAnimation(entity, skeleton, clip, SkeletalPosePlaybackMode::Clamp,
                                       std::vector<VertexWeight>{weight})) return 1;
    if (!adapter.Instances().front().skeletalAnimation.has_value() ||
        adapter.Instances().front().skeletalPalette.size() != 1U ||
        !Near(adapter.Instances().front().skeletalPalette[0].m[12], 0.0F)) return 1;

    if (!adapter.AdvanceSkeletalAnimations(0.5F)) return 1;
    if (!Near(adapter.Instances().front().skeletalPalette[0].m[12], 1.0F) ||
        !Near(adapter.Instances().front().skeletalAnimation->Time(), 0.5F)) return 1;

    const float stableTime = adapter.Instances().front().skeletalAnimation->Time();
    const float stableX = adapter.Instances().front().skeletalPalette[0].m[12];
    if (adapter.AdvanceSkeletalAnimations(-0.1F) ||
        !Near(adapter.Instances().front().skeletalAnimation->Time(), stableTime) ||
        !Near(adapter.Instances().front().skeletalPalette[0].m[12], stableX)) return 1;

    std::printf("SCENE_MESH_ANIMATION_BINDING_SMOKE_OK binding=1 palette=1 advance=1 atomic=1\n");
    return 0;
}
