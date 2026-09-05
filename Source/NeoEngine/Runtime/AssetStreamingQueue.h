#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <vulkan/vulkan.h>

namespace NeoEngine {

using AssetID = std::string;

enum class StreamState : uint8_t {
    Pending,
    Uploading,
    Ready,
    Failed
};

struct StreamRequest {
    AssetID id;
    std::string filepath;
    float priority = 5.0f;  // Distance-based or manual
    uint32_t estimatedSizeMB = 1;
    uint8_t kind = 0;  // AssetKind
};

struct StreamedAssetInfo {
    AssetID id;
    StreamState state = StreamState::Pending;
    VkDeviceMemory gpuMemory = VK_NULL_HANDLE;
    uint32_t allocatedSizeMB = 0;
    uint64_t lastAccessFrame = 0;
};

class AssetStreamingQueue {
public:
    explicit AssetStreamingQueue(uint32_t budgetMB = 1024) noexcept 
        : memoryBudgetMB_(budgetMB) {}
    
    ~AssetStreamingQueue() noexcept = default;
    
    AssetStreamingQueue(const AssetStreamingQueue&) = delete;
    AssetStreamingQueue& operator=(const AssetStreamingQueue&) = delete;

    // Enqueue asset for streaming (priority queue)
    [[nodiscard]] bool Enqueue(const StreamRequest& req) noexcept;
    
    // Check if asset is ready
    [[nodiscard]] bool IsReady(AssetID id) const noexcept;
    
    // Get GPU memory handle for ready asset
    [[nodiscard]] VkDeviceMemory GetMemory(AssetID id) const noexcept;
    
    // Set VRAM budget (triggers eviction if needed)
    void SetMemoryBudgetMB(uint32_t budgetMB) noexcept { memoryBudgetMB_ = budgetMB; }
    
    // Query resident memory
    [[nodiscard]] uint32_t GetResidentMB() const noexcept { return residentMemoryMB_; }
    
    // Query queued count
    [[nodiscard]] uint32_t GetQueuedCount() const noexcept { return streamQueue_.size(); }
    
    // Mark asset as accessed (update LRU)
    void MarkAccessed(AssetID id, uint64_t frameNumber) noexcept;
    
    // Evict least-used assets to stay within budget
    [[nodiscard]] bool EvictToBudget() noexcept;

private:
    struct PriorityCompare {
        bool operator()(const StreamRequest& a, const StreamRequest& b) const noexcept {
            return a.priority < b.priority;  // Max-heap (higher priority first)
        }
    };

    std::priority_queue<StreamRequest, std::vector<StreamRequest>, PriorityCompare> streamQueue_;
    std::unordered_map<AssetID, StreamedAssetInfo> loadedAssets_;
    mutable std::mutex mutex_;
    
    uint32_t memoryBudgetMB_ = 1024;
    uint32_t residentMemoryMB_ = 0;
};

} // namespace NeoEngine
