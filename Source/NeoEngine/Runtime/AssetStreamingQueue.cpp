#include "AssetStreamingQueue.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace NeoEngine {

bool AssetStreamingQueue::Enqueue(const StreamRequest& req) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (req.id.empty() || req.filepath.empty() || !std::isfinite(req.priority) ||
        req.estimatedSizeMB == 0 || req.estimatedSizeMB > memoryBudgetMB_ ||
        loadedAssets_.size() >= maxAssets_ || loadedAssets_.find(req.id) != loadedAssets_.end()) {
        return false;
    }

    try {
        const auto [it, inserted] = loadedAssets_.emplace(
            req.id, StreamedAssetInfo{req.id, StreamState::Pending, VK_NULL_HANDLE,
                                     req.estimatedSizeMB, 0});
        if (!inserted) return false;
        try {
            streamQueue_.push(req);
        } catch (...) {
            loadedAssets_.erase(it);
            return false;
        }
    } catch (...) {
        return false;
    }
    return true;
}

bool AssetStreamingQueue::TryDequeue(StreamRequest& out) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (streamQueue_.empty()) return false;

    const StreamRequest candidate = streamQueue_.top();
    auto it = loadedAssets_.find(candidate.id);
    if (it == loadedAssets_.end() || it->second.state != StreamState::Pending) {
        streamQueue_.pop();
        return false;
    }

    try {
        out = candidate;
    } catch (...) {
        return false;
    }
    it->second.state = StreamState::Uploading;
    streamQueue_.pop();
    return true;
}

bool AssetStreamingQueue::CompleteUpload(AssetID id, VkDeviceMemory gpuMemory,
                                          uint32_t allocatedSizeMB) noexcept {
    if (id.empty() || gpuMemory == VK_NULL_HANDLE || allocatedSizeMB == 0) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end() || it->second.state != StreamState::Uploading) return false;
    if (allocatedSizeMB > memoryBudgetMB_ ||
        residentMemoryMB_ > std::numeric_limits<uint32_t>::max() - allocatedSizeMB) return false;

    it->second.gpuMemory = gpuMemory;
    it->second.allocatedSizeMB = allocatedSizeMB;
    it->second.state = StreamState::Ready;
    residentMemoryMB_ += allocatedSizeMB;
    return true;
}

bool AssetStreamingQueue::FailUpload(AssetID id) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end() || it->second.state != StreamState::Uploading) return false;
    loadedAssets_.erase(it);
    return true;
}

bool AssetStreamingQueue::Release(AssetID id) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end()) return false;
    if (it->second.state == StreamState::Ready) residentMemoryMB_ -= it->second.allocatedSizeMB;
    loadedAssets_.erase(it);
    return true;
}

bool AssetStreamingQueue::IsReady(AssetID id) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    return it != loadedAssets_.end() && it->second.state == StreamState::Ready;
}

StreamState AssetStreamingQueue::GetState(AssetID id) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    return it == loadedAssets_.end() ? StreamState::Failed : it->second.state;
}

VkDeviceMemory AssetStreamingQueue::GetMemory(AssetID id) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end() || it->second.state != StreamState::Ready) return VK_NULL_HANDLE;
    return it->second.gpuMemory;
}

void AssetStreamingQueue::SetMemoryBudgetMB(uint32_t budgetMB) noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        memoryBudgetMB_ = budgetMB;
    }
    (void)EvictToBudget();
}

uint32_t AssetStreamingQueue::GetMemoryBudgetMB() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return memoryBudgetMB_;
}

uint32_t AssetStreamingQueue::GetResidentMB() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return residentMemoryMB_;
}

uint32_t AssetStreamingQueue::GetQueuedCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(streamQueue_.size());
}

uint32_t AssetStreamingQueue::GetAssetCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(loadedAssets_.size());
}

void AssetStreamingQueue::MarkAccessed(AssetID id, uint64_t frameNumber) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it != loadedAssets_.end() && it->second.state == StreamState::Ready) {
        it->second.lastAccessFrame = frameNumber;
    }
}

bool AssetStreamingQueue::EvictToBudget() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (residentMemoryMB_ <= memoryBudgetMB_) return true;

    std::vector<AssetID> candidates;
    try {
        candidates.reserve(loadedAssets_.size());
        for (const auto& [id, info] : loadedAssets_) {
            if (info.state == StreamState::Ready) candidates.push_back(id);
        }
        std::sort(candidates.begin(), candidates.end(), [this](const AssetID& a, const AssetID& b) {
            const auto lhs = loadedAssets_.find(a);
            const auto rhs = loadedAssets_.find(b);
            if (lhs->second.lastAccessFrame != rhs->second.lastAccessFrame)
                return lhs->second.lastAccessFrame < rhs->second.lastAccessFrame;
            return a < b;
        });
    } catch (...) {
        return false;
    }

    for (const auto& id : candidates) {
        if (residentMemoryMB_ <= memoryBudgetMB_) break;
        auto it = loadedAssets_.find(id);
        if (it == loadedAssets_.end() || it->second.state != StreamState::Ready) continue;
        residentMemoryMB_ -= it->second.allocatedSizeMB;
        loadedAssets_.erase(it);
    }
    return residentMemoryMB_ <= memoryBudgetMB_;
}

} // namespace NeoEngine
