#include "Demos/TriggerRuntimeSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine; const char* output="/tmp/fauzanengine_trigger_runtime_surface_demo_smoke.ppm"; TriggerRuntimeSurfaceDemoConfig invalid{};invalid.frames=3U;TriggerRuntimeSurfaceDemoReceipt receipt{};TriggerRuntimeSurfaceDemoError error{};if(RunTriggerRuntimeSurfaceDemo(invalid,receipt,error)||error!=TriggerRuntimeSurfaceDemoError::InvalidConfiguration)return 1;
    TriggerRuntimeSurfaceDemoConfig config{};config.width=96U;config.height=64U;config.frames=4U;config.hiddenSurface=true;config.ppmPath=output;const bool ran=RunTriggerRuntimeSurfaceDemo(config,receipt,error);if(!ran||receipt.renderedFrames!=4U||receipt.presentedFrames!=4U||receipt.visiblePixels==0U||receipt.enteredEvents!=1U||receipt.exitedEvents!=1U||receipt.frameHash==0U||receipt.finalX<0.7F){std::fprintf(stderr,"trigger-runtime ran=%d error=%u frames=%u present=%u pixels=%u enter=%u exit=%u hash=%llu x=%.3f\n",ran?1:0,static_cast<unsigned>(error),receipt.renderedFrames,receipt.presentedFrames,receipt.visiblePixels,receipt.enteredEvents,receipt.exitedEvents,static_cast<unsigned long long>(receipt.frameHash),receipt.finalX);return 1;}
    std::ifstream artifact(output,std::ios::binary);char header[2]{};if(!artifact.read(header,2)||header[0]!='P'||header[1]!='6')return 1;artifact.close();std::remove(output);std::printf("TRIGGER_RUNTIME_SURFACE_DEMO_SMOKE_OK frames=%u enter=%u exit=%u pixels=%u x=%.3f hash=%llu\n",receipt.renderedFrames,receipt.enteredEvents,receipt.exitedEvents,receipt.visiblePixels,receipt.finalX,static_cast<unsigned long long>(receipt.frameHash));return 0;
}
