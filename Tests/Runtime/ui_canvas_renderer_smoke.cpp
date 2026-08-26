#include "Runtime/SoftwareRenderer.h"
#include "Runtime/UiCanvasRenderer.h"
#include "Runtime/AssetRegistry.h"
#include "Runtime/TextureStaging.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    UiInputRouter router;
    if (!router.AddWidget({1,0,{0,0,64,64},0,false,true}) || !router.AddWidget({2,1,{16,16,32,32},1,true,true}) || router.HitTest(32,32) != 2U) { std::fprintf(stderr,"router\n"); return 1; }
    SoftwareRenderer renderer;
    if (!renderer.Initialize(64,64) || !renderer.Clear(0xFF000000U)) { std::fprintf(stderr,"renderer\n"); return 1; }
    UiCanvasRenderer canvas;
    if (!canvas.SetStyle({1,0xFF202040U}) || !canvas.SetStyle({2,0xFF00FF00U}) || !canvas.SetLabel({1,"FARM",20,20,1,0xFFFFFFFFU}) || !canvas.SetLabel({2,"HUD",2,2,1,0xFFFFFFFFU})) { std::fprintf(stderr,"setup\n"); return 1; }
    if (canvas.SetLabel({1,"COPY",4,4,1,0xFFFFFFFFU}) || canvas.LastError()!=UiCanvasError::DuplicateLabel) { std::fprintf(stderr,"duplicate\n"); return 1; }
    if (!canvas.Draw(router,renderer)) { std::fprintf(stderr,"draw=%u\n",static_cast<unsigned>(canvas.LastError())); return 1; }
    if (renderer.PixelAt(32,32) != 0xFF00FF00U || renderer.PixelAt(12,12) != 0xFF202040U || renderer.PixelAt(20,20) != 0xFF00FF00U || renderer.PixelAt(18,18) != 0xFFFFFFFFU || renderer.FrameHash()==0U) { std::fprintf(stderr,"pixels center=%08x panel=%08x covered=%08x label=%08x\n",renderer.PixelAt(32,32),renderer.PixelAt(12,12),renderer.PixelAt(20,20),renderer.PixelAt(18,18)); return 1; }
    const uint64_t labelHash = renderer.FrameHash();
    SoftwareRenderer repeatedRenderer;
    if (!repeatedRenderer.Initialize(64,64) || !repeatedRenderer.Clear(0xFF000000U) || !canvas.Draw(router,repeatedRenderer) || repeatedRenderer.FrameHash()!=labelHash) { std::fprintf(stderr,"determinism\n"); return 1; }
    UiCanvasRenderer invalidCanvas;
    if (invalidCanvas.SetLabel({3,"farm",0,0,1,0xFFFFFFFFU}) || invalidCanvas.LastError()!=UiCanvasError::InvalidLabel || !invalidCanvas.SetStyle({1,0xFF202040U}) || !invalidCanvas.SetStyle({2,0xFF00FF00U}) || !invalidCanvas.SetLabel({2,"HUD",28,28,1,0xFFFFFFFFU}) || invalidCanvas.Draw(router,renderer) || invalidCanvas.LastError()!=UiCanvasError::LabelOutsideWidget) { std::fprintf(stderr,"label_validation=%u\n",static_cast<unsigned>(invalidCanvas.LastError())); return 1; }
    UiInputRouter outside;
    UiCanvasRenderer outsideCanvas;
    if (!outside.AddWidget({3,0,{40,0,32,32},0,false,true}) || !outsideCanvas.SetStyle({3,0xFFFFFFFFU}) || outsideCanvas.Draw(outside,renderer) || outsideCanvas.LastError()!=UiCanvasError::OutsideSurface) { std::fprintf(stderr,"surface=%u\n",static_cast<unsigned>(outsideCanvas.LastError())); return 1; }
    AssetRegistry assets; const std::vector<uint8_t> iconPpm{'P','6','\n','2',' ','1','\n','2','5','5','\n',255U,0U,0U,0U,0U,255U};
    if (!assets.ImportBytes("ui.icon",AssetKind::Texture,{},iconPpm) || !assets.MarkReady("ui.icon")) return 1;
    TextureStagingStore textures; if (!textures.StagePpm(assets,"ui.icon")) return 1;
    UiInputRouter imageRouter; UiCanvasRenderer imageCanvas; SoftwareRenderer imageRenderer;
    if (!imageRouter.AddWidget({9,0,{8,8,32,16},0,false,true}) || !imageCanvas.SetStyle({9,0xFF202020U}) || !imageCanvas.SetImage(assets,{9,nullptr,textures.Find("ui.icon")}) || imageCanvas.SetImage(assets,{9,nullptr,textures.Find("ui.icon")}) || imageCanvas.LastError()!=UiCanvasError::DuplicateImage || !imageRenderer.Initialize(64,32) || !imageRenderer.Clear(0xFF000000U) || !imageCanvas.Draw(imageRouter,imageRenderer) || imageRenderer.PixelAt(12,16)!=0xFFFF0000U || imageRenderer.PixelAt(36,16)!=0xFF0000FFU) return 1;
    const uint64_t imageHash=imageRenderer.FrameHash(); const std::vector<uint8_t> greenPpm{'P','6','\n','2',' ','1','\n','2','5','5','\n',0U,255U,0U,0U,255U,0U};
    if (!assets.ReplaceBytes("ui.icon",greenPpm) || imageCanvas.Draw(imageRouter,imageRenderer) || imageCanvas.LastError()!=UiCanvasError::InvalidImage || imageRenderer.FrameHash()!=imageHash) return 1;
    UiCanvasRenderer invalidImage; if (invalidImage.SetImage(assets,{9,nullptr,textures.Find("ui.icon"),0U,2U,0U,1U,1U}) || invalidImage.LastError()!=UiCanvasError::InvalidImage) return 1;
    std::printf("UI_CANVAS_RENDERER_SMOKE_OK widgets=2 routing=1 layered=1 labels=2 image=1 atomic=1 bounds=1 hash=%llu\n", static_cast<unsigned long long>(labelHash)); return 0;
}
