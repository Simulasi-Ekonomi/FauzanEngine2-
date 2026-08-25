#include "Runtime/SceneRenderAdapter.h"
#include "Runtime/SoftwareRenderer.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    SceneWorld world; SceneEntity meshEntity{}, spriteEntity{};
    if (!world.Create(meshEntity) || !world.Create(spriteEntity) || !world.SetTransform(meshEntity,{0,0,5,0,0,0,1,1,1}) || !world.SetTransform(spriteEntity,{0,0,3,0,0,0,1,1,1}) || !world.UpdateTransforms()) return 1;
    const std::vector<MeshVertex> triangle{{{-1,-1,0},{0,0,-1},0,0},{{1,-1,0},{0,0,-1},1,0},{{0,1,0},{0,0,-1},0.5F,1}}; const std::vector<uint16_t> indices{0,1,2};
    SceneMeshAdapter meshes; if (!meshes.Add({meshEntity,triangle,indices,{0xFFCC4422U,0.2F,0.8F,nullptr}})) return 1;
    const CpuTextureResource spriteTexture{"scene-render-sprite",1U,TextureSourceFormat::PpmP6,1U,1U,{90U,230U,140U,255U}}; SceneSpriteAdapter sprites; if (!sprites.AddStaged(spriteEntity,spriteTexture,1.0F,1.0F,1,0,0xFFFFFFFFU)) return 1;
    RenderCamera camera; SoftwareRenderer renderer; if(!camera.Initialize({RenderCameraMode::Perspective,{},5,90,1,0.1F,20}) || !renderer.Initialize(64,64) || !renderer.Clear(0xFF000000U)) return 1;
    SceneRenderAdapter scene; if(!scene.Draw(world,meshes,sprites,camera,renderer,{{0,0,-1}}) || scene.LastError()!=SceneRenderAdapterError::None || renderer.PixelAt(32,32)==0xFF000000U) return 1; const uint64_t hash=renderer.FrameHash();
    SceneSpriteAdapter broken; if(!broken.AddStaged({99,1},spriteTexture,1.0F,1.0F,1,0,0xFFFFFFFFU) || !renderer.Clear(0xFF123456U)) return 1; const uint64_t prior=renderer.FrameHash(); if(scene.Draw(world,meshes,broken,camera,renderer,{{0,0,-1}}) || scene.LastError()!=SceneRenderAdapterError::SpriteQueueFailed || renderer.FrameHash()!=prior) return 1;
    SceneWorld billboardWorld; SceneEntity billboardEntity{}; if(!billboardWorld.Create(billboardEntity)||!billboardWorld.SetTransform(billboardEntity,{5,0,0,0,0,0,1,1,1})||!billboardWorld.UpdateTransforms())return 1; SceneSpriteAdapter billboardSprites; if(!billboardSprites.AddStaged(billboardEntity,spriteTexture,1,1,0,0,0xFFFFFFFFU,0.25F,true,false))return 1; RenderCamera oriented; SoftwareRenderer billboardRenderer; SpriteBatch billboardBatch; if(!oriented.Initialize({RenderCameraMode::Perspective,{},5,90,1,0.1F,20,{1,0,0},{0,1,0}})||!billboardRenderer.Initialize(64,64)||!billboardRenderer.Clear(0xFF000000U)||!billboardSprites.Queue(billboardWorld,billboardBatch)||!billboardBatch.Flush(billboardRenderer,oriented)||billboardRenderer.PixelAt(32,32)==0xFF000000U)return 1;
    std::printf("SCENE_RENDER_ADAPTER_SMOKE_OK meshSprite=1 billboard=1 atomic=1 hash=%llu\n",static_cast<unsigned long long>(hash)); return 0;
}
