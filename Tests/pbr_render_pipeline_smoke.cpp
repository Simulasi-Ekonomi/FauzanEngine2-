#include "Runtime/PBRRenderPipeline.h"

#include <cassert>
#include <iostream>

using namespace NeoEngine;

int main() {
    PBRRenderPipeline pipeline;

    // Default construction must not expose a partially initialized Vulkan pipeline.
    assert(!pipeline.IsValid());
    assert(pipeline.GetPipeline() == VK_NULL_HANDLE);
    assert(pipeline.GetPipelineLayout() == VK_NULL_HANDLE);
    assert(pipeline.GetLightingLayout() == VK_NULL_HANDLE);
    assert(pipeline.GetMaterialLayout() == VK_NULL_HANDLE);

    // The public API must reject an incomplete initialization request without creating resources.
    const VkVertexInputBindingDescription binding{0, sizeof(float) * 8, VK_VERTEX_INPUT_RATE_VERTEX};
    assert(!pipeline.Initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
                                {}, {}, binding, {}));
    assert(!pipeline.IsValid());

    pipeline.Destroy();
    assert(!pipeline.IsValid());

    std::cout << "PBR render pipeline smoke: PASS\n";
    return 0;
}
