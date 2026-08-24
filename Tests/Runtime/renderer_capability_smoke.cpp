#include "Runtime/RendererCapability.h"
#include <cstdio>
using namespace NeoEngine;
int main(){ const auto capability=RendererCapabilityProbe::Query(); const bool ok=capability.state==RendererCapabilityState::ReadyHeadless&&RendererCapabilityProbe::CanRender()&&!RendererCapabilityProbe::CanPresent()&&capability.backend=="software-raster"; if(!ok){std::fprintf(stderr,"RENDERER_CAPABILITY_SMOKE_FAIL\n");return 1;} std::printf("RENDERER_CAPABILITY_SMOKE_OK state=ready_headless\n"); }
