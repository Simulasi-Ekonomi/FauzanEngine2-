#include "Runtime/RendererCapability.h"
#include <cstdio>
using namespace NeoEngine;
int main(){ const auto capability=RendererCapabilityProbe::Query(); const bool ok=capability.state==RendererCapabilityState::ReadyPresent&&RendererCapabilityProbe::CanRender()&&RendererCapabilityProbe::CanPresent()&&capability.backend=="software-raster+sdl-surface"; if(!ok){std::fprintf(stderr,"RENDERER_CAPABILITY_SMOKE_FAIL\n");return 1;} std::printf("RENDERER_CAPABILITY_SMOKE_OK state=ready_present runtimeOptIn=1\n"); }
