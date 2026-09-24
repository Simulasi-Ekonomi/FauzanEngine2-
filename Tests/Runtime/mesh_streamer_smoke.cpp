#include "Core/GPU/MeshStreamer.h"
#include "Runtime/VulkanContext.h"

#include <array>
#include <cstdint>
#include <cstdio>

int main() {
    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::puts("MESH_STREAMER_SMOKE_SKIPPED_NO_VULKAN");
        return 0;
    }

    NeoEngine::MeshStreamer streamer;
    if (streamer.Initialize(context.Device(), context.PhysicalDevice(), 256U) == false) return 1;
    if (!streamer.IsValid() || streamer.GetBuffer() == VK_NULL_HANDLE) return 2;

    const std::array<std::uint32_t, 3> vertices{0x00000000U, 0x3f800000U, 0x40000000U};
    if (!streamer.UploadMesh(vertices.data(), sizeof(vertices))) return 3;
    if (streamer.GetUploadedBytes() != sizeof(vertices)) return 4;

    std::array<std::uint32_t, 128> oversized{};
    if (streamer.UploadMesh(oversized.data(), sizeof(oversized))) return 5;
    if (streamer.UploadMesh(nullptr, 4U)) return 6;

    streamer.Destroy();
    if (streamer.IsValid() || streamer.GetUploadedBytes() != 0U) return 7;

    context.Reset();
    std::puts("MESH_STREAMER_SMOKE_OK init=1 upload=1 bounds=1 destroy=1");
    return 0;
}
