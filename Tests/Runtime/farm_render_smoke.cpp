#include "Runtime/FarmRenderAdapter.h"
#include "Runtime/SoftwareRenderer.h"
#include "Systems/FarmSystem.h"
#include <cstdio>
using namespace NeoEngine;int main(){FarmSystem farm(2,2,100);SoftwareRenderer r;uint32_t units=0;bool ok=farm.IsReady()&&farm.Till(0,0)&&farm.Plant(0,0,FarmCrop::Wheat)&&farm.Water(0,0)&&farm.Tick(1)&&r.Initialize(96,96)&&FarmRenderAdapter::Render(farm,r)&&r.WritePpm("farm_render_smoke.ppm")&&r.FrameHash()!=0;if(!ok)return 1;std::printf("FARM_RENDER_SMOKE_OK hash=%llu\n",static_cast<unsigned long long>(r.FrameHash()));}
