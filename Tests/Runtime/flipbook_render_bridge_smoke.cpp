#include "Runtime/FlipbookRenderBridge.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/SpriteBatch.h"
#include "Runtime/TextureStaging.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    CpuTextureResource atlas{"bridge.atlas",1U,TextureSourceFormat::PpmP6,2U,1U,{255U,0U,0U,255U,0U,0U,255U,255U}};
    SceneWorld world; SceneEntity actor{}; if (!world.Create(actor) || !world.SetTransform(actor,{0.0F,0.0F,1.0F}) || !world.UpdateTransforms()) return 1;
    const Transform3 before = *world.GetTransform(actor); SceneSpriteAdapter sprites; if (!sprites.AddStaged(actor,atlas,1.0F,1.0F,0,0,0xFFFFFFFFU)) return 1;
    FlipbookPlayback playback; FlipbookFrameSelector selector; FlipbookRenderBridge bridge; SpriteBatch batch; SpriteSourceRect rect{9U,9U,9U,9U};
    if (!playback.Initialize({1.0F,true}) || !selector.Initialize({2U,1U,1U,1U,2U}) || !bridge.AdvanceQueue(playback,selector,world,sprites,batch,0.5F,rect) || rect.x != 1U || batch.Count()!=1U || std::fabs(playback.TimeSeconds()-0.5F)>0.0001F) return 1;
    const float time = playback.TimeSeconds(); const uint16_t count = batch.Count(); const SpriteSourceRect stable = rect;
    if (bridge.AdvanceQueue(playback,selector,world,sprites,batch,std::nanf(""),rect) || bridge.LastError()!=FlipbookRenderBridgeError::PlaybackFailed || playback.TimeSeconds()!=time || batch.Count()!=count || rect.x!=stable.x) return 1;
    SceneWorld missing; if (bridge.AdvanceQueue(playback,selector,missing,sprites,batch,0.25F,rect) || bridge.LastError()!=FlipbookRenderBridgeError::QueueFailed || playback.TimeSeconds()!=time || batch.Count()!=count || rect.x!=stable.x) return 1;
    const Transform3* after=world.GetTransform(actor); if(after==nullptr || after->x!=before.x || after->y!=before.y || after->z!=before.z) return 1;
    std::printf("FLIPBOOK_RENDER_BRIDGE_SMOKE_OK frame=%u batch=%u atomic=1 transform=read-only\n",rect.x,batch.Count()); return 0;
}
