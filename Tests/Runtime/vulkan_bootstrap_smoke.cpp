#include "Runtime/VulkanBootstrap.h"
#include <cstdio>
using namespace NeoEngine;int main(){auto r=VulkanBootstrap::Probe();if(!r.instanceCreated||!r.deviceCreated||!r.commandSubmitted){std::fprintf(stderr,"VULKAN_BOOTSTRAP_SMOKE_FAIL native_submit\n");return 1;}std::printf("VULKAN_BOOTSTRAP_SMOKE_OK devices=%u device=1 submit=1\n",r.physicalDeviceCount);}
