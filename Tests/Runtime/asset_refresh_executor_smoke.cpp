#include "Runtime/AssetRefreshExecutor.h"

#include "Runtime/SoftwareRenderer.h"

#include <cstdio>
#include <string>

int main() {
    using namespace NeoEngine;
    const auto bytes=[](const std::string& value){return std::vector<uint8_t>(value.begin(),value.end());};
    const std::string obj="v -1 -1 0\nv 1 -1 0\nv 0 1 0\nvn 0 0 -1\nf 1//1 2//1 3//1\n";
    const std::vector<uint8_t> redPpm{'P','6','\n','1',' ','1','\n','2','5','5','\n',255U,0U,0U};
    const std::vector<uint8_t> bluePpm{'P','6','\n','1',' ','1','\n','2','5','5','\n',0U,0U,255U};
    const std::vector<uint8_t> greenPpm{'P','6','\n','1',' ','1','\n','2','5','5','\n',0U,255U,0U};
    const std::vector<uint8_t> yellowPpm{'P','6','\n','1',' ','1','\n','2','5','5','\n',255U,255U,0U};
    const std::vector<uint8_t> badPpm{'P','6','\n','1',' ','1','\n','2','5','5','\n',0U};
    AssetRegistry registry;
    if(!registry.ImportBytes("mesh",AssetKind::Mesh,{},bytes(obj))||!registry.MarkReady("mesh")||!registry.ImportBytes("tile",AssetKind::Texture,{},redPpm)||!registry.MarkReady("tile"))return 1;
    TextureStagingStore textures;MeshStagingStore meshes;MaterialStagingStore materials;
    if(!textures.StagePpm(registry,"tile")||!meshes.StageObj(registry,"mesh"))return 1;
    const CpuMeshResource* mesh=meshes.Find("mesh");const CpuTextureResource* texture=textures.Find("tile");
    SceneWorld world;SceneEntity entity{};if(mesh==nullptr||texture==nullptr||!world.Create(entity)||!world.SetTransform(entity,{0,0,5,0,0,0,1,1,1})||!world.UpdateTransforms())return 1;
    MeshMaterial textureMaterial{0xFFFFFFFFU,0.2F,0.8F,texture};SceneMeshAdapter scene;if(!scene.AddStaged(entity,*mesh,textureMaterial))return 1;
    RenderCamera camera;if(!camera.Initialize({RenderCameraMode::Perspective,{0,0,0},5,90,1,0.1F,20}))return 1;
    auto pixel=[&](uint32_t& value){SoftwareRenderer renderer;if(!renderer.Initialize(64,64)||!renderer.Clear(0xFF000000U)||!scene.Draw(world,camera,renderer,{{0,0,-1}}))return false;value=renderer.PixelAt(32,32);return true;};uint32_t redPixel=0U;if(!pixel(redPixel)||redPixel!=0xFFFF0000U)return 1;
    AssetRefreshDiagnostics diagnostics;AssetRefreshExecutor executor;
    const uint64_t redHash=texture->sourceHash;if(!registry.ReplaceBytes("tile",bluePpm)||!diagnostics.BuildPlan(registry,"tile",textures,meshes,materials,scene)||diagnostics.Entries().size()!=2U||diagnostics.Entries()[0].action!=AssetRefreshAction::RefreshTexture||diagnostics.Entries()[1].action!=AssetRefreshAction::RebindSceneInstance||!executor.Preflight(diagnostics.Entries(),registry,textures,meshes,materials,scene)||executor.PreflightReceipts().size()!=2U||!executor.PreflightReceipts()[0].structurallyValid||!executor.PreflightReceipts()[1].structurallyValid||texture->sourceHash!=redHash||!executor.Execute(diagnostics.Entries(),registry,textures,meshes,materials,scene)||executor.LastError()!=AssetRefreshExecutorError::None||executor.Receipts().size()!=2U||!executor.Receipts()[0].succeeded||!executor.Receipts()[1].succeeded||!textures.IsCurrent(registry,"tile"))return 1;
    uint32_t bluePixel=0U;if(!pixel(bluePixel)||bluePixel!=0xFF0000FFU)return 1;
    SceneEntity spriteEntity{};SceneSpriteAdapter sprites;if(!world.Create(spriteEntity)||!world.SetTransform(spriteEntity,{0,0,3,0,0,0,1,1,1})||!world.UpdateTransforms()||!sprites.AddStaged(spriteEntity,*textures.Find("tile"),1,1,0,0,0xFFFFFFFFU))return 1;
    if(!registry.ReplaceBytes("tile",greenPpm))return 1;const uint64_t spriteHash=registry.Find("tile")->contentHash;const std::vector<AssetRefreshPlanEntry> spritePlan{{AssetRefreshAction::RefreshTexture,"tile",{}, {},spriteHash},{AssetRefreshAction::RefreshSpriteInstance,"tile",{},spriteEntity,spriteHash}};if(!executor.ExecuteSpritesAtomic(spritePlan,registry,textures,sprites)||!textures.IsCurrent(registry,"tile"))return 1;std::string spriteId;uint64_t boundHash=0U;if(!sprites.InspectStagedTexture(spriteEntity,spriteId,boundHash)||spriteId!="tile"||boundHash!=spriteHash)return 1;
    if(!registry.ReplaceBytes("tile",yellowPpm)||!diagnostics.BuildPlan(registry,"tile",textures,meshes,materials,scene)||diagnostics.Entries().size()!=2U||!executor.ExecuteAtomic(diagnostics.Entries(),registry,textures,meshes,materials,scene)||executor.LastError()!=AssetRefreshExecutorError::None||executor.Receipts().size()!=2U||!textures.IsCurrent(registry,"tile"))return 1;
    texture=textures.Find("tile");uint32_t greenPixel=0U;if(texture==nullptr||!pixel(greenPixel)||greenPixel!=0xFFFFFF00U)return 1;const uint64_t greenHash=texture->sourceHash;const std::vector<AssetRefreshReceipt> atomicReceipts=executor.Receipts();const std::vector<AssetRefreshPreflightReceipt> atomicPreflight=executor.PreflightReceipts();
    if(!registry.ReplaceBytes("tile",redPpm)||!diagnostics.BuildPlan(registry,"tile",textures,meshes,materials,scene)||diagnostics.Entries().size()!=2U||!registry.ReplaceBytes("tile",badPpm)||executor.ExecuteAtomic(diagnostics.Entries(),registry,textures,meshes,materials,scene)||executor.LastError()!=AssetRefreshExecutorError::PlanStale||executor.Receipts().size()!=atomicReceipts.size()||executor.Receipts()[0].assetId!=atomicReceipts[0].assetId||executor.PreflightReceipts().size()!=atomicPreflight.size()||texture->sourceHash!=greenHash||textures.IsCurrent(registry,"tile"))return 1;
    if(!diagnostics.BuildPlan(registry,"tile",textures,meshes,materials,scene)||diagnostics.Entries().size()!=2U)return 1;std::vector<AssetRefreshPlanEntry> duplicatePlan=diagnostics.Entries();duplicatePlan.push_back(duplicatePlan[0]);if(executor.Preflight(duplicatePlan,registry,textures,meshes,materials,scene)||executor.LastError()!=AssetRefreshExecutorError::PlanInvalid||!executor.PreflightReceipts().empty()||executor.Execute(duplicatePlan,registry,textures,meshes,materials,scene)||executor.LastError()!=AssetRefreshExecutorError::PlanInvalid||!executor.Receipts().empty()||texture->sourceHash!=greenHash||textures.IsCurrent(registry,"tile"))return 1;
    if(!diagnostics.BuildPlan(registry,"tile",textures,meshes,materials,scene)||diagnostics.Entries().size()!=2U||executor.Preflight(diagnostics.Entries(),registry,textures,meshes,materials,scene)||executor.LastError()!=AssetRefreshExecutorError::ProbeFailed||executor.PreflightReceipts().size()!=1U||executor.PreflightReceipts()[0].structurallyValid||executor.Execute(diagnostics.Entries(),registry,textures,meshes,materials,scene)||executor.LastError()!=AssetRefreshExecutorError::ProbeFailed||!executor.Receipts().empty()||texture->sourceHash!=greenHash||textures.IsCurrent(registry,"tile"))return 1;
    const std::vector<AssetRefreshPlanEntry> invalidPlan{{AssetRefreshAction::RefreshTexture,"missing",{}, {},1U}};if(executor.Preflight(invalidPlan,registry,textures,meshes,materials,scene)||executor.LastError()!=AssetRefreshExecutorError::PlanStale||executor.PreflightReceipts().size()!=1U||executor.PreflightReceipts()[0].structurallyValid||texture->sourceHash!=greenHash)return 1;
    uint32_t retainedGreen=0U;if(!pixel(retainedGreen)||retainedGreen!=greenPixel)return 1;
    std::printf("ASSET_REFRESH_EXECUTOR_SMOKE_OK explicit=1 atomic=1 receiptsPreserved=1 planHash=1 planInvalid=1 preflight=1 probe=1 receipts=2 copiedTexture=1 mutation=caller\n");
    return 0;
}
