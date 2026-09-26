#include "Runtime/RuntimeAssetStreamBridge.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <thread>
#include <type_traits>
#include <vector>

namespace {
VkDeviceMemory FakeDeviceMemory(uintptr_t value) {
    if constexpr (std::is_pointer_v<VkDeviceMemory>) {
        return reinterpret_cast<VkDeviceMemory>(value);
    } else {
        return static_cast<VkDeviceMemory>(value);
    }
}
}

int main() {
    using namespace NeoEngine;
    const char* path = "runtime_asset_stream_bridge_smoke.ppm";
    {
        std::ofstream file(path, std::ios::binary);
        const char header[] = "P6\n1 1\n255\n";
        const unsigned char pixel[] = {255U, 32U, 16U};
        file.write(header, sizeof(header) - 1U);
        file.write(reinterpret_cast<const char*>(pixel), sizeof(pixel));
    }

    AssetRegistry registry;
    AssetResourceManager resources(registry);
    StreamManager streams(1, 8, 1024U, 1024U);
    AssetStreamingQueue queue(16U, 8U);
    RuntimeAssetStreamBridge bridge(registry, resources, streams, queue);

    assert(bridge.Start());

    StreamRequest request{};
    request.id = "smoke.texture";
    request.filepath = path;
    request.priority = 10.0F;
    request.estimatedSizeMB = 1U;
    request.kind = static_cast<uint8_t>(AssetKind::Texture);
    assert(bridge.Request(request));

    for (uint32_t i = 0U; i < 100U && bridge.PendingGpuUploadCount() == 0U; ++i) {
        bridge.Pump();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    bridge.Pump();

    AssetResourceHandle handle{};
    StreamRequest upload{};
    assert(bridge.GetGpuUpload(request.id, upload, handle));
    assert(upload.id == request.id);
    assert(queue.GetState(request.id) == StreamState::Uploading);

    AssetResourceReceipt receipt{};
    assert(resources.Query(handle, receipt));
    assert(receipt.gpuUploadsInFlight == 1U);
    assert(!receipt.gpuResident);

    std::vector<VkDeviceMemory> released;
    const VkDeviceMemory firstMemory = FakeDeviceMemory(0x1001U);
    assert(bridge.CompleteGpuUpload(
        request.id, firstMemory, 1U,
        [&released](VkDeviceMemory memory) { released.push_back(memory); }));
    assert(bridge.PendingGpuUploadCount() == 0U);
    assert(bridge.ResidentGpuUploadCount() == 1U);
    assert(queue.IsReady(request.id));
    assert(released.empty());

    assert(bridge.Refresh(request));
    // Two-phase refresh keeps the old resident GPU ownership valid until the
    // replacement upload completes or is explicitly cancelled.
    assert(bridge.ResidentGpuUploadCount() == 1U);
    assert(bridge.PendingGpuUploadCount() == 1U);
    assert(released.empty());

    bool refreshed = false;
    for (uint32_t i = 0U; i < 100U && !refreshed; ++i) {
        bridge.Pump();
        refreshed = queue.GetState(request.id) == StreamState::Uploading;
        if (!refreshed) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    assert(refreshed);

    AssetResourceHandle refreshedHandle{};
    StreamRequest refreshedRequest{};
    assert(bridge.GetGpuUpload(request.id, refreshedRequest, refreshedHandle));
    assert(queue.IsRefreshing(request.id));
    assert(refreshedRequest.id == request.id);
    assert(refreshedHandle == handle);

    assert(bridge.FailGpuUpload(request.id));
    assert(bridge.PendingGpuUploadCount() == 0U);
    assert(bridge.ResidentGpuUploadCount() == 1U);
    assert(queue.IsReady(request.id));
    assert(released.empty());

    // A later explicit release still invokes the original resident owner exactly once.
    assert(bridge.ReleaseGpuUpload(request.id));
    assert(bridge.ResidentGpuUploadCount() == 0U);
    assert(released.size() == 1U && released[0] == firstMemory);

    bridge.Stop();
    std::remove(path);
    std::puts("RUNTIME_ASSET_STREAM_BRIDGE_SMOKE_OK refresh=1 ownership_release=1 gpu_completion_left_authoritative=1");
    return 0;
}
