#include "AssetStreamingQueue.h"
#include <algorithm>

namespace NeoEngine {

bool AssetStreamingQueue::Enqueue(const StreamRequest& req) noexcept {
    if (req.id.empty() || req.filepath.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already loaded
    auto it = loadedAssets_.find(req.id);
    if (it != loadedAssets_.end()) {
        return true;  // Already in system
    }

    // Enqueue for streaming
    streamQueue_.push(req);
    loadedAssets_[req.id] = {
        req.id,
        StreamState::Pending,
        VK_NULL_HANDLE,
        req.estimatedSizeMB,
        0
    };

    return true;
}

bool AssetStreamingQueue::IsReady(AssetID id) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end()) return false;
    return it->second.state == StreamState::Ready;
}

VkDeviceMemory AssetStreamingQueue::GetMemory(AssetID id) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end()) return VK_NULL_HANDLE;
    if (it->second.state != StreamState::Ready) return VK_NULL_HANDLE;
    return it->second.gpuMemory;
}

void AssetStreamingQueue::MarkAccessed(AssetID id, uint64_t frameNumber) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it != loadedAssets_.end()) {
        it->second.lastAccessFrame = frameNumber;
    }
}

bool AssetStreamingQueue::EvictToBudget() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (residentMemoryMB_ <= memoryBudgetMB_) {
        return true;  // Within budget
    }

    // Simple LRU eviction: remove least-recently-used assets
    std::vector<AssetID> candidates;
    for (auto& [id, info] : loadedAssets_) {
        if (info.state == StreamState::Ready) {
            candidates.push_back(id);
        }
    }

    // Sort by last access frame (ascending = oldest first)
    std::sort(candidates.begin(), candidates.end(),
              [this](const AssetID& a, const AssetID& b) {
                  return loadedAssets_[a].lastAccessFrame < loadedAssets_[b].lastAccessFrame;
              });

    // Evict until within budget
    for (const auto& id : candidates) {
        if (residentMemoryMB_ <= memoryBudgetMB_) break;
        
        auto it = loadedAssets_.find(id);
        if (it != loadedAssets_.end()) {
            residentMemoryMB_ -= it->second.allocatedSizeMB;
            loadedAssets_.erase(it);
        }
    }

    return residentMemoryMB_ <= memoryBudgetMB_;
}

} // namespace NeoEngine
