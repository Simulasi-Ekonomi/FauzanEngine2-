#include "Runtime/NeoRuntime.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace {
NeoEngine::Mat4 Identity() { return NeoEngine::Mat4::Identity(); }
NeoEngine::SkeletalPoseKeyframe Key(float time, float x) {
    NeoEngine::SkeletalPoseKeyframe key{};
    key.time=time;key.translation={x,0.0F,0.0F};return key;
}
NeoEngine::SceneMeshInstance Mesh(NeoEngine::SceneEntity entity) {
    using namespace NeoEngine;
    SceneMeshInstance mesh{};
    mesh.entity=entity;
    mesh.vertices={
        {{-0.2F,-0.2F,0.0F},{0.0F,0.0F,1.0F},0.0F,0.0F},
        {{0.2F,-0.2F,0.0F},{0.0F,0.0F,1.0F},1.0F,0.0F},
        {{0.0F,0.2F,0.0F},{0.0F,0.0F,1.0F},0.5F,1.0F}
    };
    for(auto& vertex:mesh.vertices)vertex.boneWeights={1.0F,0.0F,0.0F,0.0F};
    mesh.indices={0,1,2};return mesh;
}
bool Near(float a,float b){return std::fabs(a-b)<0.0001F;}
float RootX(const NeoEngine::SceneMeshAdapter& meshes,NeoEngine::SceneEntity entity){
    for(const auto& instance:meshes.Instances())if(instance.entity==entity&&!instance.skeletalPalette.empty())return instance.skeletalPalette[0].m[12];
    return std::numeric_limits<float>::quiet_NaN();
}
}
int main(){
    using namespace NeoEngine;
    NeoRuntime runtime;RuntimeConfig config{};
    if(!runtime.Initialize(config)||runtime.Scene()==nullptr||runtime.SceneMeshes()==nullptr)return 1;
    SceneEntity left{},right{};
    if(!runtime.Scene()->Create(left)||!runtime.Scene()->Create(right))return 2;
    if(!runtime.Scene()->SetTransform(left,{0.0F,0.0F,3.0F,0.0F,0.0F,0.0F,1.0F,1.0F,1.0F})||
       !runtime.Scene()->SetTransform(right,{0.0F,0.0F,3.0F,0.0F,0.0F,0.0F,1.0F,1.0F,1.0F})||
       !runtime.Scene()->UpdateTransforms())return 3;
    if(!runtime.SceneMeshes()->Add(Mesh(left))||!runtime.SceneMeshes()->Add(Mesh(right)))return 4;
    Skeleton skeleton;Bone root{"root",-1};root.localBindPose=Identity();
    if(!skeleton.TryAddBone(root))return 5;
    SkeletalPoseClip slow,fast;
    if(!slow.Configure(1U)||!slow.SetTrack(0U,{Key(0.0F,0.0F),Key(1.0F,1.0F)})||
       !fast.Configure(1U)||!fast.SetTrack(0U,{Key(0.0F,0.0F),Key(1.0F,2.0F)}))return 6;
    if(!runtime.BindSceneSkeletalAnimation(left,skeleton,slow)||
       !runtime.BindSceneSkeletalAnimation(right,skeleton,fast)||
       runtime.SceneSkeletalAnimationCount()!=2U)return 7;
    if(runtime.BindSceneSkeletalAnimation({0xFFFFU,0U},skeleton,slow)||
       runtime.SetSceneSkeletalAnimationSpeed(left,std::numeric_limits<float>::quiet_NaN()))return 8;
    if(!runtime.Tick())return 9;
    const float firstLeft=RootX(*runtime.SceneMeshes(),left),firstRight=RootX(*runtime.SceneMeshes(),right);
    if(!Near(firstLeft,1.0F/60.0F)||!Near(firstRight,2.0F/60.0F)||Near(firstLeft,firstRight))return 10;
    if(!runtime.SetSceneSkeletalAnimationPaused(right,true)||!runtime.Tick())return 11;
    const float secondLeft=RootX(*runtime.SceneMeshes(),left),secondRight=RootX(*runtime.SceneMeshes(),right);
    if(!Near(secondLeft,2.0F/60.0F)||!Near(secondRight,firstRight))return 12;
    Mat4 replacement=Mat4::Identity();replacement.m[12]=9.0F;
    std::vector<SceneSkeletalPaletteUpdate> invalidBatch{{left,{replacement}},{{0xFFFEU,0U},{replacement}}};
    if(runtime.SceneMeshes()->SetSkeletalPalettesAtomic(invalidBatch)||!Near(RootX(*runtime.SceneMeshes(),left),secondLeft))return 13;
    if(!runtime.UnbindSceneSkeletalAnimation(right)||runtime.SceneSkeletalAnimationCount()!=1U||
       !runtime.SceneMeshes()->Instances()[1].skeletalPalette.empty())return 14;
    if(!runtime.UnbindSceneSkeletalAnimation(left)||runtime.SceneSkeletalAnimationCount()!=0U)return 15;
    if(!runtime.Shutdown())return 16;
    std::puts("NEO_RUNTIME_SCENE_ANIMATION_SMOKE_OK per_entity=1 independent_time=1 pause=1 atomic_palette_commit=1 unbind=1");
    return 0;
}
