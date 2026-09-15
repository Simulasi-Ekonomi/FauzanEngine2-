#include "Runtime/PBRRenderPipeline.h"

#include <cassert>
#include <iostream>

using namespace NeoEngine;

int main() {
    PBRRenderPipeline pipeline;

    assert(!pipeline.IsValid());
    assert(!pipeline.HasIBL());
    assert(pipeline.GetPipeline() == VK_NULL_HANDLE);
    assert(pipeline.GetPipelineLayout() == VK_NULL_HANDLE);
    assert(pipeline.GetLightingLayout() == VK_NULL_HANDLE);
    assert(pipeline.GetMaterialLayout() == VK_NULL_HANDLE);
    assert(pipeline.GetIBLLayout() == VK_NULL_HANDLE);

    const VkVertexInputBindingDescription binding{0, sizeof(float) * 8, VK_VERTEX_INPUT_RATE_VERTEX};
    assert(!pipeline.Initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
                                {}, {}, binding, {}));
    assert(!pipeline.IsValid());

    assert(!pipeline.InitializeWithIBL(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
                                       VK_NULL_HANDLE, {}, {}, binding, {}));
    assert(!pipeline.IsValid());
    assert(!pipeline.HasIBL());

    pipeline.Destroy();
    assert(!pipeline.IsValid());
    assert(!pipeline.HasIBL());

    std::cout << "PBR render pipeline smoke: PASS\n";
    return 0;
}
