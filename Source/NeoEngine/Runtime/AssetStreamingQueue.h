#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace NeoEngine {

using AssetID = std::string;

enum class StreamState : uint8_t { Pending, Uploading, Ready, Failed };

struct StreamRequest {
    AssetID id;
    std::string filepath;
    float priority = 5.0f;
    uint32_t estimatedSizeMB = 1;
    uint8_t kind = 0;
};

struct StreamedAssetInfo {
    AssetID id;
    StreamState state = StreamState::Pending;
    VkDeviceMemory gpuMemory = VK_NULL_HANDLE;
    uint32_t allocatedSizeMB = 0;
    uint64_t lastAccessFrame = 0;
    bool replacingResident = false;
    std::function<void(VkDeviceMemory)> gpuMemoryReleaseCallback{};
};

class AssetStreamingQueue {
public:
    using GpuMemoryReleaseCallback = std::function<void(VkDeviceMemory)>;

    explicit AssetStreamingQueue(uint32_t budgetMB = 1024, uint32_t maxAssets = 4096) noexcept
        : memoryBudgetMB_(budgetMB), maxAssets_(maxAssets) {}

    ~AssetStreamingQueue() noexcept;

    AssetStreamingQueue(const AssetStreamingQueue&) = delete;
    AssetStreamingQueue& operator=(const AssetStreamingQueue&) = delete;

    [[nodiscard]] bool Enqueue(const StreamRequest& req) noexcept;
    // Cancels a request still waiting in the priority queue. Once dequeued,
    // cancellation belongs to the upload owner and this call returns false.
    [[nodiscard]] bool CancelPending(AssetID id) noexcept;
    // Transitions one specific pending asset to Uploading without disturbing
    // higher-priority requests. This is the ownership-safe handoff used when an
    // asynchronous file stream completes out of priority order.
    [[nodiscard]] bool BeginUpload(AssetID id, StreamRequest& out) noexcept;
    // Starts a refresh while retaining the currently resident GPU allocation.
    // The old allocation remains owned until CompleteUpload commits the replacement.
    [[nodiscard]] bool BeginRefresh(const StreamRequest& request) noexcept;
    [[nodiscard]] bool TryDequeue(StreamRequest& out) noexcept;
    [[nodiscard]] bool CompleteUpload(AssetID id, VkDeviceMemory gpuMemory,
                                      uint32_t allocatedSizeMB) noexcept;
    // Preferred ownership-safe form: the release callback is captured on this
    // allocation, so concurrent assets cannot inherit another upload's owner.
    [[nodiscard]] bool CompleteUpload(AssetID id, VkDeviceMemory gpuMemory,
                                      uint32_t allocatedSizeMB,
                                      GpuMemoryReleaseCallback releaseCallback) noexcept;
    // Completes a resident refresh and returns the old allocation's release owner.
    // The caller must invoke the returned callback exactly once after the replacement
    // has been committed and its completion fence observed.
    [[nodiscard]] bool CompleteRefreshUpload(AssetID id, VkDeviceMemory gpuMemory,
                                             uint32_t allocatedSizeMB,
                                             GpuMemoryReleaseCallback releaseCallback,
                                             GpuMemoryReleaseCallback& oldReleaseCallback,
                                             VkDeviceMemory& oldGpuMemory) noexcept;
    [[nodiscard]] bool FailUpload(AssetID id) noexcept;
    [[nodiscard]] bool Release(AssetID id) noexcept;

    [[nodiscard]] bool IsReady(AssetID id) const noexcept;
    [[nodiscard]] bool IsRefreshing(AssetID id) const noexcept;
    [[nodiscard]] StreamState GetState(AssetID id) const noexcept;
    [[nodiscard]] VkDeviceMemory GetMemory(AssetID id) const noexcept;

    void SetMemoryBudgetMB(uint32_t budgetMB) noexcept;
    [[nodiscard]] uint32_t GetMemoryBudgetMB() const noexcept;
    [[nodiscard]] uint32_t GetResidentMB() const noexcept;
    [[nodiscard]] uint32_t GetQueuedCount() const noexcept;
    [[nodiscard]] uint32_t GetAssetCount() const noexcept;

    void MarkAccessed(AssetID id, uint64_t frameNumber) noexcept;
    [[nodiscard]] bool EvictToBudget() noexcept;

    // The queue stores ownership metadata for VkDeviceMemory. Every accepted
    // GPU allocation must have a release callback so Release/Evict/destruction
    // always have a real Vulkan ownership path. Test handles use the same explicit
    // owner contract; CompleteUpload fails closed when no callback is installed.
    // Each ready allocation captures the callback active when its upload is
    // accepted; replacing the queue callback never changes existing ownership.
    void SetGpuMemoryReleaseCallback(GpuMemoryReleaseCallback callback) noexcept;
    [[nodiscard]] bool HasGpuMemoryReleaseCallback() const noexcept;

private:
    struct PriorityCompare {
        bool operator()(const StreamRequest& a, const StreamRequest& b) const noexcept {
            if (a.priority != b.priority) return a.priority < b.priority;
            if (a.id != b.id) return a.id > b.id;
            return a.filepath > b.filepath;
        }
    };

    [[nodiscard]] bool ReleaseGpuMemoryLocked(StreamedAssetInfo& info) noexcept;

    std::priority_queue<StreamRequest, std::vector<StreamRequest>, PriorityCompare> streamQueue_;
    std::unordered_map<AssetID, StreamedAssetInfo> loadedAssets_;
    mutable std::mutex mutex_;

    uint32_t memoryBudgetMB_ = 1024;
    uint32_t residentMemoryMB_ = 0;
    uint32_t maxAssets_ = 4096;
    GpuMemoryReleaseCallback gpuMemoryReleaseCallback_{};
};

} // namespace NeoEngine
