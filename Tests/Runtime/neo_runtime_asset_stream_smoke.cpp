#include "Runtime/NeoRuntime.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

int main() {
    using namespace NeoEngine;
    const char* path = "neo_runtime_asset_stream_smoke.bin";
    {
        std::ofstream file(path, std::ios::binary);
        const char header[] = "P6\n1 1\n255\n";
        const unsigned char pixel[] = {16U, 32U, 48U};
        file.write(header, sizeof(header) - 1U);
        file.write(reinterpret_cast<const char*>(pixel), sizeof(pixel));
    }

    NeoRuntime runtime;
    RuntimeConfig config{};
    config.farmNpcCount = 1U;
    if (!runtime.Initialize(config)) {
        std::fprintf(stderr, "RUNTIME_INIT_FAIL error=%u\\n", static_cast<unsigned>(runtime.LastError()));
        std::remove(path);
        return 1;
    }

    StreamRequest request{};
    request.id = "runtime.stream.smoke";
    request.filepath = path;
    request.priority = 9.0F;
    request.estimatedSizeMB = 1U;
    request.kind = static_cast<uint8_t>(AssetKind::Texture);
    assert(runtime.RequestStreamedAsset(request));

    bool ready = false;
    for (uint32_t i = 0U; i < 120U && !ready; ++i) {
        assert(runtime.Tick());
        const AssetRegistry* assets = runtime.Assets();
        if (assets == nullptr) {
            std::fprintf(stderr, "ASSET_REGISTRY_MISSING state=%u\\n", static_cast<unsigned>(runtime.State()));
            runtime.Shutdown();
            std::remove(path);
            return 1;
        }
        const AssetDefinition* definition = assets->Find(request.id);
        ready = definition != nullptr && definition->state == AssetState::Ready;
        if (!ready) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    assert(ready);
    AssetResourceReceipt receipt{};
    assert(runtime.Resources()->Query(request.id, receipt));
    assert(receipt.refCount == 1U);
    assert(receipt.gpuUploadsInFlight == 1U);
    assert(!receipt.gpuResident);

    assert(runtime.CancelStreamedAsset(request.id) == false);
    assert(runtime.Shutdown());
    std::remove(path);
    std::puts("NEO_RUNTIME_ASSET_STREAM_SMOKE_OK file_to_registry=1 resource_pin=1");
    return 0;
}
