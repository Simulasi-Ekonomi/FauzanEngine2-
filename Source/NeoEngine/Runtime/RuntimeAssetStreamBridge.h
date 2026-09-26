#pragma once

#include "AssetRegistry.h"
#include "AssetResourceManager.h"
#include "AssetStreamingQueue.h"
#include "StreamManager.h"
#include "BmpTexture.h"
#include "PpmTexture.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace NeoEngine {

// Runtime-thread bridge between real file I/O and the canonical asset/resource
// systems. StreamManager callbacks never mutate registry/resource/queue state;
// Pump() performs those mutations on the runtime thread.
class RuntimeAssetStreamBridge {
public:
    RuntimeAssetStreamBridge(AssetRegistry& assets,
                             AssetResourceManager& resources,
                             StreamManager& streams,
                             AssetStreamingQueue& queue) noexcept;

    RuntimeAssetStreamBridge(const RuntimeAssetStreamBridge&) = delete;
    RuntimeAssetStreamBridge& operator=(const RuntimeAssetStreamBridge&) = delete;

    [[nodiscard]] bool Start() noexcept;
    void Stop() noexcept;

    [[nodiscard]] bool Request(const StreamRequest& request) noexcept;
    [[nodiscard]] bool Cancel(const AssetID& id) noexcept;

    // Commits successful file loads into AssetRegistry/AssetResourceManager and
    // advances the queue from Pending to Uploading. No GPU residency is published
    // here; that remains owned by the authoritative Vulkan completion path.
    [[nodiscard]] uint32_t Pump(uint32_t maxRequests = 32U) noexcept;

    [[nodiscard]] bool GetGpuUpload(AssetID id, StreamRequest& request,
                                    AssetResourceHandle& handle) const noexcept;
    [[nodiscard]] bool GetGpuUploadTextureData(AssetID id, std::vector<uint8_t>& rgba,
                                               uint32_t& width, uint32_t& height) const noexcept;
    [[nodiscard]] bool GetPendingGpuUploadIds(std::vector<AssetID>& ids) const noexcept;

    // Called only after the real GPU completion fence has been observed. The
    // release callback becomes the queue's explicit owner of the accepted memory.
    [[nodiscard]] bool CompleteGpuUpload(
        AssetID id, VkDeviceMemory gpuMemory, uint32_t allocatedSizeMB,
        AssetStreamingQueue::GpuMemoryReleaseCallback releaseCallback) noexcept;

    [[nodiscard]] bool FailGpuUpload(AssetID id) noexcept;
    [[nodiscard]] bool ReleaseGpuUpload(AssetID id) noexcept;
    [[nodiscard]] bool ReleaseAllGpuUploads() noexcept;
    [[nodiscard]] uint32_t PendingGpuUploadCount() const noexcept;
    [[nodiscard]] uint32_t ResidentGpuUploadCount() const noexcept;

private:
    struct GpuTexturePayload {
        std::vector<uint8_t> rgba;
        uint32_t width = 0U;
        uint32_t height = 0U;
    };

    struct LoadedEvent {
        AssetID id;
        StreamRequest request;
        std::vector<uint8_t> bytes;
        bool success = false;
    };

    [[nodiscard]] static AssetKind ToAssetKind(uint8_t kind) noexcept;
    [[nodiscard]] bool QueueLoadedEvent(LoadedEvent event) noexcept;

    AssetRegistry& assets_;
    AssetResourceManager& resources_;
    StreamManager& streams_;
    AssetStreamingQueue& queue_;

    mutable std::mutex mutex_;
    std::vector<LoadedEvent> events_;
    std::unordered_map<AssetID, AssetResourceHandle> gpuUploads_;
    std::unordered_map<AssetID, AssetResourceHandle> residentGpuUploads_;
    std::unordered_map<AssetID, GpuTexturePayload> gpuTexturePayloads_;
    std::unordered_map<AssetID, StreamRequest> requests_;
    bool started_ = false;
};

} // namespace NeoEngine
