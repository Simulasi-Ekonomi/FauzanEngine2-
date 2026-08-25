#include "Demos/MotionAnimationSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine; const char* output="/tmp/fauzanengine_motion_animation_surface_demo_smoke.ppm"; MotionAnimationSurfaceDemoConfig invalid{}; invalid.frames=3U; MotionAnimationSurfaceDemoReceipt receipt{}; MotionAnimationSurfaceDemoError error{};
    if(RunMotionAnimationSurfaceDemo(invalid,receipt,error)||error!=MotionAnimationSurfaceDemoError::InvalidConfiguration)return 1;
    MotionAnimationSurfaceDemoConfig config{}; config.width=96U; config.height=64U; config.frames=4U; config.hiddenSurface=true; config.ppmPath=output;
    if(!RunMotionAnimationSurfaceDemo(config,receipt,error)||receipt.renderedFrames!=4U||receipt.presentedFrames!=4U||receipt.visiblePixels==0U||receipt.frameHash==0U||receipt.finalX<=0.0F||receipt.sampledAnimation!=0.0F||receipt.endedLocomoting||receipt.locomotionTintFrames!=2U||receipt.tintHash==0U)return 1;
    std::ifstream artifact(output,std::ios::binary);char header[2]{};if(!artifact.read(header,2)||header[0]!='P'||header[1]!='6')return 1;artifact.close();std::remove(output);std::printf("MOTION_ANIMATION_SURFACE_DEMO_SMOKE_OK frames=%u presented=%u pixels=%u x=%.3f hash=%llu\n",receipt.renderedFrames,receipt.presentedFrames,receipt.visiblePixels,receipt.finalX,static_cast<unsigned long long>(receipt.frameHash));return 0;
}
