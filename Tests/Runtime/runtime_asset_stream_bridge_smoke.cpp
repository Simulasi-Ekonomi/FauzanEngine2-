#include "Runtime/RuntimeAssetStreamBridge.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

int main() {
    using namespace NeoEngine;
    const char* path = "runtime_asset_stream_bridge_smoke.bin";
    {
        std::ofstream file(path, std::ios::binary);
        const unsigned char bytes[] = {1U, 2U, 3U, 4U};
        file.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
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
    assert(bridge.FailGpuUpload(request.id));
    assert(queue.GetState(request.id) == StreamState::Failed);
    assert(bridge.PendingGpuUploadCount() == 0U);

    bridge.Stop();
    std::remove(path);
    std::puts("RUNTIME_ASSET_STREAM_BRIDGE_SMOKE_OK file_to_resource_pin=1 gpu_completion_left_authoritative=1");
    return 0;
}
