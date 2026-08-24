#include "Runtime/VulkanShaderProbe.h"
#include <cstdio>
using namespace NeoEngine;int main(){if(!VulkanShaderProbe::CreateTriangleModules())return 1;std::printf("VULKAN_SHADER_SMOKE_OK modules=2\n");}
