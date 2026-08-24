#include "Runtime/AssetRefreshDiagnostics.h"

#include <cstdio>
#include <string>

int main() {
    using namespace NeoEngine;
    const auto bytes=[](const std::string& value){return std::vector<uint8_t>(value.begin(),value.end());};
    const std::string obj="v -1 -1 0\nv 1 -1 0\nv 0 1 0\nvn 0 0 -1\nf 1//1 2//1 3//1\n";
    const std::string mtl="newmtl grass\nKd 0.1 0.8 0.2\nd 1\n";
    const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',10U,20U,30U};
    AssetRegistry registry;
    if(!registry.ImportBytes("mesh",AssetKind::Mesh,{},bytes(obj))||!registry.MarkReady("mesh")||!registry.ImportBytes("material",AssetKind::Material,{"mesh"},bytes(mtl))||!registry.MarkReady("material")||!registry.ImportBytes("tile",AssetKind::Texture,{},ppm)||!registry.MarkReady("tile"))return 1;
    TextureStagingStore textures;MeshStagingStore meshes;MaterialStagingStore materials;
    if(!textures.StagePpm(registry,"tile")||!meshes.StageObj(registry,"mesh")||!materials.StageMtl(registry,"material","grass"))return 1;
    SceneWorld world;SceneEntity entity{},textureEntity{};if(!world.Create(entity)||!world.Create(textureEntity)||!world.SetTransform(entity,{0,0,5,0,0,0,1,1,1})||!world.SetTransform(textureEntity,{0,0,5,0,0,0,1,1,1})||!world.UpdateTransforms())return 1;
    const CpuMeshResource* mesh=meshes.Find("mesh");const CpuMaterialResource* material=materials.Find("material","grass");const CpuTextureResource* texture=textures.Find("tile");SceneMeshAdapter scene;MeshMaterial textureMaterial{0xFFFFFFFFU,0.2F,0.8F,texture};if(mesh==nullptr||material==nullptr||texture==nullptr||!scene.AddStaged(entity,*mesh,*material)||!scene.AddStaged(textureEntity,*mesh,textureMaterial))return 1;
    AssetRefreshDiagnostics diagnostics;
    if(!diagnostics.BuildPlan(registry,"mesh",textures,meshes,materials,scene)||!diagnostics.Entries().empty())return 1;
    const uint64_t meshHash=mesh->sourceHash;if(!registry.ReplaceBytes("mesh",bytes(obj+"# replacement\n"))||meshes.IsCurrent(registry,"mesh")||!diagnostics.BuildPlan(registry,"mesh",textures,meshes,materials,scene)||diagnostics.Entries().size()!=3U||diagnostics.Entries()[0].action!=AssetRefreshAction::RefreshMesh||diagnostics.Entries()[0].assetId!="mesh"||diagnostics.Entries()[1].action!=AssetRefreshAction::RebindSceneInstance||diagnostics.Entries()[1].entity!=entity||diagnostics.Entries()[2].action!=AssetRefreshAction::RebindSceneInstance||diagnostics.Entries()[2].entity!=textureEntity||mesh->sourceHash!=meshHash||meshes.IsCurrent(registry,"mesh"))return 1;
    if(!meshes.Refresh(registry,"mesh")||!scene.RefreshStaged(entity,*mesh,*material)||!scene.RefreshStaged(textureEntity,*mesh,textureMaterial))return 1;const uint64_t materialHash=material->sourceHash;const std::string replacementMtl="newmtl grass\nKd 0.8 0.1 0.2\nd 1\n";if(!registry.ReplaceBytes("material",bytes(replacementMtl))||materials.IsCurrent(registry,"material","grass")||!diagnostics.BuildPlan(registry,"material",textures,meshes,materials,scene)||diagnostics.Entries().size()!=2U||diagnostics.Entries()[0].action!=AssetRefreshAction::RefreshMaterial||diagnostics.Entries()[0].assetId!="material"||diagnostics.Entries()[0].materialName!="grass"||diagnostics.Entries()[1].action!=AssetRefreshAction::RebindSceneInstance||diagnostics.Entries()[1].entity!=entity||material->sourceHash!=materialHash||materials.IsCurrent(registry,"material","grass"))return 1;
    const uint64_t textureHash=texture->sourceHash;const std::vector<uint8_t> replacementPpm{'P','6','\n','1',' ','1','\n','2','5','5','\n',40U,50U,60U};if(!registry.ReplaceBytes("tile",replacementPpm)||textures.IsCurrent(registry,"tile")||!diagnostics.BuildPlan(registry,"tile",textures,meshes,materials,scene)||diagnostics.Entries().size()!=2U||diagnostics.Entries()[0].action!=AssetRefreshAction::RefreshTexture||diagnostics.Entries()[0].assetId!="tile"||diagnostics.Entries()[1].action!=AssetRefreshAction::RebindSceneInstance||diagnostics.Entries()[1].entity!=textureEntity||texture->sourceHash!=textureHash||textures.IsCurrent(registry,"tile"))return 1;
    if(diagnostics.BuildPlan(registry,"missing",textures,meshes,materials,scene)||diagnostics.LastError()!=AssetRefreshDiagnosticsError::MissingAsset||diagnostics.Entries().size()!=2U)return 1;
    std::printf("ASSET_REFRESH_DIAGNOSTICS_SMOKE_OK actions=3 meshRefresh=1 materialRefresh=1 textureRefresh=1 rebind=1 mutation=0\n");
    return 0;
}
