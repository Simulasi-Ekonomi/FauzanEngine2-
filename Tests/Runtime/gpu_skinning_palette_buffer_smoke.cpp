#include "Animation/GPUSkinningPaletteBuffer.h"
#include "Runtime/VulkanContext.h"

#include <cassert>
#include <vector>

int main() {
    NeoEngine::VulkanContext context;
    if (!context.Initialize()) return 0;

    NeoEngine::GPUSkinningPaletteBuffer palette;
    assert(palette.Initialize(context.Device(), context.PhysicalDevice()));
    assert(palette.IsValid());

    std::vector<NeoEngine::Mat4> bones(2);
    for (size_t i = 0; i < bones.size(); ++i) {
        bones[i] = {};
        bones[i].m[0] = bones[i].m[5] = bones[i].m[10] = bones[i].m[15] = 1.0F;
        bones[i].m[12] = static_cast<float>(i);
    }
    assert(palette.UploadPalette(bones));
    assert(palette.BoneCount() == bones.size());

    palette.Destroy();
    assert(!palette.IsValid());
    context.Reset();
    return 0;
}
