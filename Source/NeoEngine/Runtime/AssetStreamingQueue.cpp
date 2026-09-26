#include "AssetStreamingQueue.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace NeoEngine {

AssetStreamingQueue::~AssetStreamingQueue() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, info] : loadedAssets_) (void)ReleaseGpuMemoryLocked(info);
    loadedAssets_.clear();
    residentMemoryMB_ = 0;
}

bool AssetStreamingQueue::ReleaseGpuMemoryLocked(StreamedAssetInfo& info) noexcept {
    if (info.gpuMemory == VK_NULL_HANDLE) return true;
    const VkDeviceMemory memory = info.gpuMemory;
    if (info.gpuMemoryReleaseCallback) {
        try {
            info.gpuMemoryReleaseCallback(memory);
        } catch (...) {
            return false;
        }
    }
    info.gpuMemory = VK_NULL_HANDLE;
    info.allocatedSizeMB = 0;
    info.gpuMemoryReleaseCallback = {};
    return true;
}

bool AssetStreamingQueue::Enqueue(const StreamRequest& req) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (req.id.empty() || req.filepath.empty() || !std::isfinite(req.priority) ||
        req.estimatedSizeMB == 0 || req.estimatedSizeMB > memoryBudgetMB_ ||
        loadedAssets_.size() >= maxAssets_ || loadedAssets_.find(req.id) != loadedAssets_.end()) return false;
    try {
        const auto [it, inserted] = loadedAssets_.emplace(req.id, StreamedAssetInfo{req.id, StreamState::Pending, VK_NULL_HANDLE, req.estimatedSizeMB, 0, {}});
        if (!inserted) return false;
        try { streamQueue_.push(req); } catch (...) { loadedAssets_.erase(it); return false; }
    } catch (...) { return false; }
    return true;
}

bool AssetStreamingQueue::CancelPending(AssetID id) noexcept {
    if (id.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const auto asset = loadedAssets_.find(id);
    if (asset == loadedAssets_.end() || asset->second.state != StreamState::Pending) return false;
    decltype(streamQueue_) filtered;
    try {
        auto remaining = streamQueue_;
        while (!remaining.empty()) {
            StreamRequest request = remaining.top();
            remaining.pop();
            if (request.id != id) filtered.push(std::move(request));
        }
    } catch (...) {
        return false;
    }
    streamQueue_.swap(filtered);
    loadedAssets_.erase(asset);
    return true;
}

bool AssetStreamingQueue::BeginUpload(AssetID id, StreamRequest& out) noexcept {
    if (id.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto asset = loadedAssets_.find(id);
    if (asset == loadedAssets_.end() || asset->second.state != StreamState::Pending) return false;

    decltype(streamQueue_) remaining;
    decltype(streamQueue_) working;
    StreamRequest selected{};
    bool found = false;
    try {
        working = streamQueue_;
        while (!working.empty()) {
            StreamRequest candidate = working.top();
            working.pop();
            if (!found && candidate.id == id) {
                selected = std::move(candidate);
                found = true;
            } else {
                remaining.push(std::move(candidate));
            }
        }
        if (!found) return false;
        out = selected;
        streamQueue_.swap(remaining);
        asset->second.state = StreamState::Uploading;
        return true;
    } catch (...) {
        // The original queue and Pending state are untouched until every
        // allocation/copy above has succeeded.
        return false;
    }
}

bool AssetStreamingQueue::BeginRefresh(AssetID id, StreamRequest& out) noexcept {
    if (id.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end() || it->second.state != StreamState::Ready ||
        it->second.gpuMemory == VK_NULL_HANDLE || it->second.allocatedSizeMB == 0U ||
        it->second.replacingResident) return false;

    try {
        StreamRequest selected{};
        selected.id = id;
        selected.filepath = id;
        // The bridge supplies the real request after validating its own request map.
        // Only the state transition belongs here; no queue item is inserted because
        // refresh file I/O is owned by StreamManager.
        out = std::move(selected);
        it->second.state = StreamState::Uploading;
        it->second.replacingResident = true;
        return true;
    } catch (...) {
        return false;
    }
}

bool AssetStreamingQueue::TryDequeue(StreamRequest& out) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!streamQueue_.empty()) {
        StreamRequest candidate;
        try {
            candidate = streamQueue_.top();
        } catch (...) {
            return false;
        }
        auto it = loadedAssets_.find(candidate.id);
        if (it == loadedAssets_.end() || it->second.state != StreamState::Pending) { streamQueue_.pop(); continue; }
        // Copy the request before changing queue state so an allocation failure
        // cannot strand the asset in Uploading while leaving its queue entry live.
        try { out = candidate; } catch (...) { return false; }
        it->second.state = StreamState::Uploading;
        streamQueue_.pop();
        return true;
    }
    return false;
}

bool AssetStreamingQueue::CompleteUpload(AssetID id, VkDeviceMemory gpuMemory, uint32_t allocatedSizeMB) noexcept {
    GpuMemoryReleaseCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = gpuMemoryReleaseCallback_;
    }
    return CompleteUpload(id, gpuMemory, allocatedSizeMB, std::move(callback));
}

bool AssetStreamingQueue::CompleteRefreshUpload(AssetID id, VkDeviceMemory gpuMemory,
                                            uint32_t allocatedSizeMB,
                                            GpuMemoryReleaseCallback releaseCallback,
                                            GpuMemoryReleaseCallback& oldReleaseCallback) noexcept {
    oldReleaseCallback = {};
    if (id.empty() || gpuMemory == VK_NULL_HANDLE || allocatedSizeMB == 0U || !releaseCallback) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end() || it->second.state != StreamState::Uploading ||
        !it->second.replacingResident || it->second.gpuMemory == VK_NULL_HANDLE ||
        it->second.allocatedSizeMB == 0U) return false;

    const uint32_t oldSize = it->second.allocatedSizeMB;
    if (residentMemoryMB_ < oldSize ||
        allocatedSizeMB > memoryBudgetMB_ ||
        residentMemoryMB_ - oldSize > memoryBudgetMB_ - allocatedSizeMB) return false;

    GpuMemoryReleaseCallback previous = std::move(it->second.gpuMemoryReleaseCallback);
    try {
        it->second.gpuMemoryReleaseCallback = std::move(releaseCallback);
    } catch (...) {
        it->second.gpuMemoryReleaseCallback = std::move(previous);
        return false;
    }
    it->second.gpuMemory = gpuMemory;
    it->second.allocatedSizeMB = allocatedSizeMB;
    it->second.state = StreamState::Ready;
    it->second.replacingResident = false;
    residentMemoryMB_ = residentMemoryMB_ - oldSize + allocatedSizeMB;
    oldReleaseCallback = std::move(previous);
    return true;
}

bool AssetStreamingQueue::CompleteUpload(AssetID id, VkDeviceMemory gpuMemory,
                                         uint32_t allocatedSizeMB,
                                         GpuMemoryReleaseCallback releaseCallback) noexcept {
    if (id.empty() || gpuMemory == VK_NULL_HANDLE || allocatedSizeMB == 0 || !releaseCallback) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end() || it->second.state != StreamState::Uploading) return false;
    if (allocatedSizeMB > memoryBudgetMB_ || residentMemoryMB_ > std::numeric_limits<uint32_t>::max() - allocatedSizeMB ||
        residentMemoryMB_ + allocatedSizeMB > memoryBudgetMB_) return false;
    try {
        it->second.gpuMemoryReleaseCallback = std::move(releaseCallback);
    } catch (...) {
        return false;
    }
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
    if (it->second.replacingResident) {
        it->second.state = StreamState::Ready;
        it->second.replacingResident = false;
        return true;
    }
    loadedAssets_.erase(it);
    return true;
}

bool AssetStreamingQueue::Release(AssetID id) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it == loadedAssets_.end()) return false;
    if (it->second.state == StreamState::Uploading && it->second.replacingResident) return false;
    if (it->second.state == StreamState::Ready) {
        const uint32_t allocationMB = it->second.allocatedSizeMB;
        if (allocationMB > residentMemoryMB_) return false;
        if (!ReleaseGpuMemoryLocked(it->second)) return false;
        residentMemoryMB_ -= allocationMB;
    }
    loadedAssets_.erase(it);
    return true;
}

bool AssetStreamingQueue::IsRefreshing(AssetID id) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = loadedAssets_.find(id);
    return it != loadedAssets_.end() && it->second.state == StreamState::Uploading && it->second.replacingResident;
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
    { std::lock_guard<std::mutex> lock(mutex_); memoryBudgetMB_ = budgetMB; }
    (void)EvictToBudget();
}

uint32_t AssetStreamingQueue::GetMemoryBudgetMB() const noexcept { std::lock_guard<std::mutex> lock(mutex_); return memoryBudgetMB_; }
uint32_t AssetStreamingQueue::GetResidentMB() const noexcept { std::lock_guard<std::mutex> lock(mutex_); return residentMemoryMB_; }
uint32_t AssetStreamingQueue::GetQueuedCount() const noexcept { std::lock_guard<std::mutex> lock(mutex_); return streamQueue_.size() > std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() : static_cast<uint32_t>(streamQueue_.size()); }
uint32_t AssetStreamingQueue::GetAssetCount() const noexcept { std::lock_guard<std::mutex> lock(mutex_); return loadedAssets_.size() > std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() : static_cast<uint32_t>(loadedAssets_.size()); }

void AssetStreamingQueue::MarkAccessed(AssetID id, uint64_t frameNumber) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loadedAssets_.find(id);
    if (it != loadedAssets_.end() && it->second.state == StreamState::Ready) it->second.lastAccessFrame = frameNumber;
}

bool AssetStreamingQueue::EvictToBudget() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (residentMemoryMB_ <= memoryBudgetMB_) return true;
    std::vector<AssetID> candidates;
    try {
        candidates.reserve(loadedAssets_.size());
        for (const auto& [id, info] : loadedAssets_) if (info.state == StreamState::Ready) candidates.push_back(id);
        std::sort(candidates.begin(), candidates.end(), [this](const AssetID& a, const AssetID& b) {
            const auto lhs = loadedAssets_.find(a), rhs = loadedAssets_.find(b);
            if (lhs->second.lastAccessFrame != rhs->second.lastAccessFrame) return lhs->second.lastAccessFrame < rhs->second.lastAccessFrame;
            return a < b;
        });
    } catch (...) { return false; }
    for (const auto& id : candidates) {
        if (residentMemoryMB_ <= memoryBudgetMB_) break;
        auto it = loadedAssets_.find(id);
        if (it == loadedAssets_.end() || it->second.state != StreamState::Ready) continue;
        const uint32_t allocationMB = it->second.allocatedSizeMB;
        if (allocationMB > residentMemoryMB_) continue;
        if (!ReleaseGpuMemoryLocked(it->second)) continue;
        residentMemoryMB_ -= allocationMB;
        loadedAssets_.erase(it);
    }
    return residentMemoryMB_ <= memoryBudgetMB_;
}

void AssetStreamingQueue::SetGpuMemoryReleaseCallback(GpuMemoryReleaseCallback callback) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    gpuMemoryReleaseCallback_ = std::move(callback);
}

bool AssetStreamingQueue::HasGpuMemoryReleaseCallback() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<bool>(gpuMemoryReleaseCallback_);
}

} // namespace NeoEngine
