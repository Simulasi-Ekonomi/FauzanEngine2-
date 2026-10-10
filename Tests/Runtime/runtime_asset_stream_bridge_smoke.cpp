#include "Runtime/RuntimeAssetStreamBridge.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

namespace {
VkDeviceMemory FakeDeviceMemory(uintptr_t value) {
    return reinterpret_cast<VkDeviceMemory>(value);
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
    auto fail = [&](int code) {
        bridge.Stop();
        std::remove(path);
        return code;
    };

    if (!bridge.Start()) return fail(1);

    StreamRequest request{};
    request.id = "smoke.texture";
    request.filepath = path;
    request.priority = 10.0F;
    request.estimatedSizeMB = 1U;
    request.kind = static_cast<uint8_t>(AssetKind::Texture);
    if (!bridge.Request(request)) return fail(2);

    for (uint32_t i = 0U; i < 100U && bridge.PendingGpuUploadCount() == 0U; ++i) {
        bridge.Pump();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    bridge.Pump();

    AssetResourceHandle handle{};
    StreamRequest upload{};
    if (!bridge.GetGpuUpload(request.id, upload, handle) ||
        upload.id != request.id ||
        queue.GetState(request.id) != StreamState::Uploading) return fail(3);

    AssetResourceReceipt receipt{};
    if (!resources.Query(handle, receipt) ||
        receipt.gpuUploadsInFlight != 1U ||
        receipt.gpuResident) return fail(4);

    std::vector<VkDeviceMemory> released;
    const VkDeviceMemory firstMemory = FakeDeviceMemory(0x1001U);
    if (!bridge.CompleteGpuUpload(
            request.id, firstMemory, 1U,
            [&released](VkDeviceMemory memory) { released.push_back(memory); })) return fail(5);
    if (bridge.PendingGpuUploadCount() != 0U ||
        bridge.ResidentGpuUploadCount() != 1U ||
        !queue.IsReady(request.id) ||
        !released.empty()) return fail(6);

    AssetResourceReceipt beforeRefresh{};
    if (!resources.Query(handle, beforeRefresh)) return fail(7);
    const uint64_t firstContentHash = beforeRefresh.contentHash;

    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        const char header[] = "P6\n1 1\n255\n";
        const unsigned char pixel[] = {8U, 64U, 192U};
        file.write(header, sizeof(header) - 1U);
        file.write(reinterpret_cast<const char*>(pixel), sizeof(pixel));
    }

    if (!bridge.Refresh(request) ||
        bridge.ResidentGpuUploadCount() != 1U ||
        bridge.PendingGpuUploadCount() != 1U ||
        !released.empty()) return fail(8);

    // BeginRefresh marks the queue Uploading immediately. Wait for Pump() to
    // publish the decoded refresh payload, which is the actual GPU-upload handoff.
    bool refreshed = false;
    for (uint32_t i = 0U; i < 100U && !refreshed; ++i) {
        bridge.Pump();
        std::vector<uint8_t> rgba;
        uint32_t width = 0U;
        uint32_t height = 0U;
        refreshed = bridge.GetGpuUploadTextureData(request.id, rgba, width, height) &&
                    !rgba.empty() && width == 1U && height == 1U;
        if (!refreshed) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if (!refreshed) return fail(9);

    AssetResourceHandle refreshedHandle{};
    StreamRequest refreshedRequest{};
    if (!bridge.GetGpuUpload(request.id, refreshedRequest, refreshedHandle) ||
        !queue.IsRefreshing(request.id) ||
        refreshedRequest.id != request.id ||
        refreshedHandle != handle) return fail(10);

    std::vector<VkDeviceMemory> replacementReleased;
    const VkDeviceMemory secondMemory = FakeDeviceMemory(0x2002U);
    const bool replacementComplete = bridge.CompleteGpuUpload(
        request.id, secondMemory, 1U,
        [&replacementReleased](VkDeviceMemory memory) { replacementReleased.push_back(memory); });
    if (!replacementComplete) {
        AssetResourceReceipt failedReceipt{};
        (void)resources.Query(handle, failedReceipt);
        std::fprintf(stderr,
            "REFRESH_COMPLETE_FAIL queueState=%u resident=%u pending=%u resourceError=%u inFlight=%u residentGpu=%d\n",
            static_cast<unsigned>(queue.GetState(request.id)),
            bridge.ResidentGpuUploadCount(), bridge.PendingGpuUploadCount(),
            static_cast<unsigned>(resources.LastError()),
            static_cast<unsigned>(failedReceipt.gpuUploadsInFlight),
            failedReceipt.gpuResident ? 1 : 0);
        return fail(11);
    }
    if (bridge.PendingGpuUploadCount() != 0U ||
        bridge.ResidentGpuUploadCount() != 1U ||
        !queue.IsReady(request.id) ||
        released.size() != 1U || released[0] != firstMemory ||
        !replacementReleased.empty()) return fail(12);

    AssetResourceReceipt afterRefresh{};
    if (!resources.Query(handle, afterRefresh) ||
        !afterRefresh.gpuResident ||
        afterRefresh.gpuUploadsInFlight != 0U ||
        afterRefresh.contentHash == firstContentHash) return fail(13);

    // Failure of a subsequent refresh must preserve the newly resident version.
    if (!bridge.Refresh(request) || bridge.PendingGpuUploadCount() != 1U) return fail(14);
    bool refreshingAgain = false;
    for (uint32_t i = 0U; i < 100U && !refreshingAgain; ++i) {
        bridge.Pump();
        std::vector<uint8_t> rgba;
        uint32_t width = 0U;
        uint32_t height = 0U;
        refreshingAgain = bridge.GetGpuUploadTextureData(request.id, rgba, width, height) &&
                          !rgba.empty() && width == 1U && height == 1U;
        if (!refreshingAgain) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if (!refreshingAgain || !queue.IsRefreshing(request.id)) return fail(15);
    if (!bridge.FailGpuUpload(request.id) ||
        bridge.PendingGpuUploadCount() != 0U ||
        bridge.ResidentGpuUploadCount() != 1U ||
        !queue.IsReady(request.id)) return fail(16);

    if (!bridge.ReleaseGpuUpload(request.id) ||
        bridge.ResidentGpuUploadCount() != 0U ||
        released.size() != 1U || released[0] != firstMemory ||
        replacementReleased.size() != 1U || replacementReleased[0] != secondMemory) return fail(17);

    bridge.Stop();
    std::remove(path);
    std::puts("RUNTIME_ASSET_STREAM_BRIDGE_SMOKE_OK refresh=1 ownership_release=1 gpu_completion_left_authoritative=1");
    return 0;
}
