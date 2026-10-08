#include <cassert>\n#include <cstdio>\n#include "Animation/GPUSkinningPaletteBuffer.h"
#include "Runtime/VulkanContext.h"

#include <cassert>
#include <vector>

int main() {
    auto require = [](bool condition, const char* message) -> bool {
        if (!condition) std::fprintf(stderr, "GPU_SKINNING_PALETTE_SMOKE_FAIL: %s\\n", message);
        return condition;
    };
    NeoEngine::VulkanContext context;
    if (!require(context.Initialize(), "VulkanContext::Initialize")) return 1;

    NeoEngine::GPUSkinningPaletteBuffer palette;
    if (!require(!palette.UploadPalette({}), "empty upload must fail") ||
        !require(palette.BoneCount() == 0U, "empty upload changed bone count") ||
        !require(palette.Initialize(context.Device(), context.PhysicalDevice()), "palette initialize") ||
        !require(palette.IsValid(), "palette invalid after initialize")) return 2;

    std::vector<NeoEngine::Mat4> bones(2);
    for (size_t i = 0; i < bones.size(); ++i) {
        bones[i] = {};
        bones[i].m[0] = bones[i].m[5] = bones[i].m[10] = bones[i].m[15] = 1.0F;
        bones[i].m[12] = static_cast<float>(i);
    }
    if (!require(palette.UploadPalette(bones), "two-bone upload") ||
        !require(palette.IsValid(), "palette invalid after upload") ||
        !require(palette.GetBuffer() != VK_NULL_HANDLE, "buffer handle missing") ||
        !require(palette.BoneCount() == bones.size(), "bone count after two-bone upload")) return 3;
    std::vector<NeoEngine::Mat4> oneBone(1);
    oneBone[0] = bones[0];
    if (!require(palette.UploadPalette(oneBone), "one-bone upload") ||
        !require(palette.BoneCount() == 1U, "bone count after one-bone upload")) return 4;
    std::vector<NeoEngine::Mat4> tooMany(NeoEngine::GPUSkinningPaletteBuffer::kMaxBones + 1U);
    if (!require(!palette.UploadPalette(tooMany), "oversized upload must fail") ||
        !require(palette.BoneCount() == 1U, "oversized upload changed bone count") ||
        !require(palette.IsValid(), "oversized upload invalidated palette")) return 5;

    palette.Destroy();
    if (!require(!palette.IsValid(), "palette valid after destroy") ||
        !require(palette.GetBuffer() == VK_NULL_HANDLE, "buffer handle survives destroy") ||
        !require(palette.BoneCount() == 0U, "bone count survives destroy") ||
        !require(!palette.UploadPalette(oneBone), "upload accepted after destroy")) return 6;
    palette.Destroy();
    if (!require(!palette.IsValid(), "second destroy resurrected palette")) return 7;
    context.Reset();
    return 0;
}
