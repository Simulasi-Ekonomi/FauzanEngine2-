#pragma once
#include <cstdint>
namespace NeoEngine { struct VulkanProbeResult{bool instanceCreated=false,deviceCreated=false,commandSubmitted=false;uint32_t physicalDeviceCount=0;};class VulkanBootstrap{public:static VulkanProbeResult Probe();};}
