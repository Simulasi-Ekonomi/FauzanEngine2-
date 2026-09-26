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

    AssetResourceReceipt beforeRefresh{};
    assert(resources.Query(handle, beforeRefresh));
    const uint64_t firstContentHash = beforeRefresh.contentHash;

    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        const char header[] = "P6\n1 1\n255\n";
        const unsigned char pixel[] = {8U, 64U, 192U};
        file.write(header, sizeof(header) - 1U);
        file.write(reinterpret_cast<const char*>(pixel), sizeof(pixel));
    }

    assert(bridge.Refresh(request));
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

    std::vector<VkDeviceMemory> replacementReleased;
    const VkDeviceMemory secondMemory = FakeDeviceMemory(0x2002U);
    assert(bridge.CompleteGpuUpload(
        request.id, secondMemory, 1U,
        [&replacementReleased](VkDeviceMemory memory) { replacementReleased.push_back(memory); }));
    assert(bridge.PendingGpuUploadCount() == 0U);
    assert(bridge.ResidentGpuUploadCount() == 1U);
    assert(queue.IsReady(request.id));
    assert(released.size() == 1U && released[0] == firstMemory);
    assert(replacementReleased.empty());

    AssetResourceReceipt afterRefresh{};
    assert(resources.Query(handle, afterRefresh));
    assert(afterRefresh.gpuResident && afterRefresh.gpuUploadsInFlight == 0U);
    assert(afterRefresh.contentHash != firstContentHash);

    // Failure of a subsequent refresh must preserve the newly resident version.
    assert(bridge.Refresh(request));
    assert(bridge.PendingGpuUploadCount() == 1U);
    bool refreshingAgain = false;
    for (uint32_t i = 0U; i < 100U && !refreshingAgain; ++i) {
        bridge.Pump();
        refreshingAgain = queue.GetState(request.id) == StreamState::Uploading;
        if (!refreshingAgain) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    assert(refreshingAgain);
    assert(bridge.FailGpuUpload(request.id));
    assert(bridge.PendingGpuUploadCount() == 0U);
    assert(bridge.ResidentGpuUploadCount() == 1U);
    assert(queue.IsReady(request.id));

    assert(bridge.ReleaseGpuUpload(request.id));
    assert(bridge.ResidentGpuUploadCount() == 0U);
    assert(released.size() == 1U && released[0] == firstMemory);
    assert(replacementReleased.size() == 1U && replacementReleased[0] == secondMemory);

    bridge.Stop();
    std::remove(path);
    std::puts("RUNTIME_ASSET_STREAM_BRIDGE_SMOKE_OK refresh=1 ownership_release=1 gpu_completion_left_authoritative=1");
    return 0;
}
