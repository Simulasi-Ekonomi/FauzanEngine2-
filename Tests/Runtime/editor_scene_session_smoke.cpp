#include "Runtime/EditorSceneSession.h"
#include "Runtime/SoftwareRenderer.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',90U,230U,140U}; AssetRegistry assets; if(!assets.ImportBytes("session.sprite",AssetKind::Texture,{},ppm)||!assets.MarkReady("session.sprite"))return 1;
    EditorSceneDocument document{EditorSceneDocument::kVersion,"editor-session",1,{{10,0,EditorSceneActorKind::Sprite,{0,0,3,0,0,0,1,1,1},"session.sprite","","","",1,1,0,0,0xFFFFFFFFU}}}; EditorSceneSession session; if(!session.Open(document,assets)||session.LastError()!=EditorSceneSessionError::None)return 1;
    const auto hierarchy=session.HierarchySnapshot(); EditorSceneActor inspected{}; EditorSceneDocument saved{}; if(hierarchy.size()!=1U||hierarchy[0].id!=10U||!session.InspectActor(10,inspected)||inspected.assetId!="session.sprite"||!session.Save(saved)||saved.sceneId!=document.sceneId)return 1;
    RenderCamera camera; SoftwareRenderer renderer; if(!camera.Initialize({RenderCameraMode::Perspective,{},5,90,1,0.1F,20})||!renderer.Initialize(64,64)||!renderer.Clear(0xFF000000U)||!session.RenderViewport(camera,renderer,{{0,0,-1}})||renderer.PixelAt(32,32)==0xFF000000U)return 1; const uint64_t hash=renderer.FrameHash();
    if(!session.UpdateTransform(10,{0.5F,0,3,0,0,0,1,1,1},assets)||!session.Save(saved)||saved.revision!=2U||!renderer.Clear(0xFF000000U)||!session.RenderViewport(camera,renderer,{{0,0,-1}})||renderer.FrameHash()==hash)return 1; const uint64_t movedHash=renderer.FrameHash();
    if(session.UpdateTransform(10,{0,0,3,0,0,0,0,1,1},assets)||session.LastError()!=EditorSceneSessionError::DocumentLoadFailed||!renderer.Clear(0xFF000000U)||!session.RenderViewport(camera,renderer,{{0,0,-1}})||renderer.FrameHash()!=movedHash)return 1;
    EditorSceneDocument invalid=document; invalid.revision=2; invalid.actors[0].assetId="missing.sprite"; if(session.Open(invalid,assets)||session.LastError()!=EditorSceneSessionError::DocumentLoadFailed||!renderer.Clear(0xFF000000U)||!session.RenderViewport(camera,renderer,{{0,0,-1}})||renderer.FrameHash()!=movedHash)return 1;
    std::printf("EDITOR_SCENE_SESSION_SMOKE_OK hierarchy=1 inspector=1 save=1 viewport=1 edit=1 atomicOpen=1 hash=%llu\n",static_cast<unsigned long long>(movedHash)); return 0;
}
