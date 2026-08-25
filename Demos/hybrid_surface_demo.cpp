#include "Demos/HybridSurfaceDemo.h"

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) { NeoEngine::HybridSurfaceDemoConfig config; for (int index=1; index<argc; ++index) { const std::string arg=argv[index]; if (arg=="--visible") config.hiddenSurface=false; else if (arg=="--frames" && index+1<argc) config.frames=static_cast<uint32_t>(std::strtoul(argv[++index],nullptr,10)); else if (arg=="--output" && index+1<argc) config.ppmPath=argv[++index]; else return 2; } NeoEngine::HybridSurfaceDemoReceipt receipt{}; NeoEngine::HybridSurfaceDemoError error{}; if(!NeoEngine::RunHybridSurfaceDemo(config,receipt,error)){std::printf("HYBRID_SURFACE_DEMO_FAIL error=%u\n",static_cast<unsigned>(error));return 1;} std::printf("HYBRID_SURFACE_DEMO_OK frames=%u presented=%u visible=%u hash=%llu\n",receipt.renderedFrames,receipt.presentedFrames,receipt.visiblePixels,static_cast<unsigned long long>(receipt.frameHash));return 0; }
