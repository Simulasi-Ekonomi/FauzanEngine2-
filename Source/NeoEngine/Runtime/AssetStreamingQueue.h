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
    [[nodiscard]] bool TryDequeue(StreamRequest& out) noexcept;
    [[nodiscard]] bool CompleteUpload(AssetID id, VkDeviceMemory gpuMemory,
                                      uint32_t allocatedSizeMB) noexcept;
    [[nodiscard]] bool FailUpload(AssetID id) noexcept;
    [[nodiscard]] bool Release(AssetID id) noexcept;

    [[nodiscard]] bool IsReady(AssetID id) const noexcept;
    [[nodiscard]] StreamState GetState(AssetID id) const noexcept;
    [[nodiscard]] VkDeviceMemory GetMemory(AssetID id) const noexcept;

    void SetMemoryBudgetMB(uint32_t budgetMB) noexcept;
    [[nodiscard]] uint32_t GetMemoryBudgetMB() const noexcept;
    [[nodiscard]] uint32_t GetResidentMB() const noexcept;
    [[nodiscard]] uint32_t GetQueuedCount() const noexcept;
    [[nodiscard]] uint32_t GetAssetCount() const noexcept;

    void MarkAccessed(AssetID id, uint64_t frameNumber) noexcept;
    [[nodiscard]] bool EvictToBudget() noexcept;

    // The queue stores ownership metadata for VkDeviceMemory. Production Vulkan
    // owners must bind a release callback so Release/Evict/destruction actually
    // return the allocation to the Vulkan device. Existing callers may leave this
    // unset when they use non-owning/test handles. Each ready allocation captures
    // the callback active when its upload is accepted; replacing the queue callback
    // never changes ownership of an existing allocation.
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
