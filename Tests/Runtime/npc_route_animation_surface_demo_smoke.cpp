#include "Demos/NpcRouteAnimationSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine;
    NpcRouteAnimationSurfaceDemoReceipt receipt{}; NpcRouteAnimationSurfaceDemoError error=NpcRouteAnimationSurfaceDemoError::None;
    if(RunNpcRouteAnimationSurfaceDemo({96U,96U,4U,true,"/tmp/npc_route_animation_invalid.ppm"},receipt,error) || error!=NpcRouteAnimationSurfaceDemoError::InvalidConfiguration) return 1;
    const char* path="/tmp/npc_route_animation_surface_demo.ppm";
    if(!RunNpcRouteAnimationSurfaceDemo({96U,96U,5U,true,path},receipt,error) || error!=NpcRouteAnimationSurfaceDemoError::None || receipt.renderedFrames!=5U || receipt.presentedFrames!=5U || receipt.visiblePixels==0U || receipt.frameHash==0U || receipt.finalX!=1.0F || receipt.finalZ!=2.0F || receipt.finalAnimationSample!=0.0F || !receipt.reachedGoal || receipt.endedLocomoting || receipt.routeTintFrames!=4U || receipt.tintHash==0U) return 1;
    std::ifstream artifact(path,std::ios::binary); char magic[2]{}; if(!artifact.read(magic,2) || magic[0]!='P' || magic[1]!='6') return 1;
    std::printf("NPC_ROUTE_ANIMATION_SURFACE_DEMO_SMOKE_OK frames=%u tint=%u goal=%d hash=%llu\n",receipt.renderedFrames,receipt.routeTintFrames,receipt.reachedGoal?1:0,static_cast<unsigned long long>(receipt.frameHash));
    return 0;
}
