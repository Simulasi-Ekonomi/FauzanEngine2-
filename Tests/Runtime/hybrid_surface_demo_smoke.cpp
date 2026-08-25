#include "Demos/HybridSurfaceDemo.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

int main() { using namespace NeoEngine; HybridSurfaceDemoReceipt receipt{}; HybridSurfaceDemoError error{}; const std::filesystem::path artifact="/tmp/hybrid_surface_demo_smoke.ppm"; std::filesystem::remove(artifact); if(RunHybridSurfaceDemo({64,64,0,true,artifact.string()},receipt,error)||error!=HybridSurfaceDemoError::InvalidConfiguration)return 1; if(!RunHybridSurfaceDemo({64,64,3,true,artifact.string()},receipt,error)||error!=HybridSurfaceDemoError::None||receipt.renderedFrames!=3U||receipt.presentedFrames!=3U||receipt.visiblePixels==0U||receipt.frameHash==0U)return 1; std::ifstream input(artifact,std::ios::binary); char magic[2]{}; input.read(magic,2); if(!input||magic[0]!='P'||magic[1]!='6'){std::filesystem::remove(artifact);return 1;} std::filesystem::remove(artifact); std::printf("HYBRID_SURFACE_DEMO_SMOKE_OK frames=%u presented=%u visible=%u hash=%llu\n",receipt.renderedFrames,receipt.presentedFrames,receipt.visiblePixels,static_cast<unsigned long long>(receipt.frameHash)); return 0; }
