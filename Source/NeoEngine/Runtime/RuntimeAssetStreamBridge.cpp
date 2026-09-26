#include "RuntimeAssetStreamBridge.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace NeoEngine {

RuntimeAssetStreamBridge::RuntimeAssetStreamBridge(
    AssetRegistry& assets, AssetResourceManager& resources,
    StreamManager& streams, AssetStreamingQueue& queue) noexcept
    : assets_(assets), resources_(resources), streams_(streams), queue_(queue) {}

bool RuntimeAssetStreamBridge::Start() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (started_) return true;
    streams_.Start();
    started_ = true;
    return true;
}

void RuntimeAssetStreamBridge::Stop() noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_) return;
        started_ = false;
    }
    streams_.Stop();

    std::vector<AssetID> ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ids.reserve(gpuUploads_.size() + residentGpuUploads_.size());
        for (const auto& [id, handle] : gpuUploads_) {
            (void)handle;
            ids.push_back(id);
        }
        for (const auto& [id, handle] : residentGpuUploads_) {
            (void)handle;
            ids.push_back(id);
        }
        requests_.clear();
        events_.clear();
    }
    // Pending uploads must already have been failed/drained by the renderer;
    // completed uploads must be explicitly released before renderer/device teardown.
    // Do not erase ownership maps here: doing so would strand queue-owned callbacks.
}

bool RuntimeAssetStreamBridge::Request(const StreamRequest& request) noexcept {
    if (request.id.empty() || request.filepath.empty() ||
        request.estimatedSizeMB == 0U) return false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_ || requests_.contains(request.id)) return false;
        if (!queue_.Enqueue(request)) return false;
        requests_.emplace(request.id, request);
    }

    const bool accepted = streams_.RequestLoad(
        request.filepath, static_cast<int>(request.priority),
        [this, request](const std::vector<uint8_t>& bytes) {
            LoadedEvent event{};
            event.id = request.id;
            event.request = request;
            event.bytes = bytes;
            event.success = true;
            (void)QueueLoadedEvent(std::move(event));
        },
        [this, request](bool success) {
            if (success) return;
            LoadedEvent event{};
            event.id = request.id;
            event.request = request;
            event.success = false;
            (void)QueueLoadedEvent(std::move(event));
        });

    if (!accepted) {
        std::lock_guard<std::mutex> lock(mutex_);
        requests_.erase(request.id);
        (void)queue_.CancelPending(request.id);
        return false;
    }
    return true;
}

bool RuntimeAssetStreamBridge::Cancel(const AssetID& id) noexcept {
    StreamRequest request{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = requests_.find(id);
        if (it == requests_.end()) return false;
        request = it->second;
    }

    const bool cancelled = streams_.Cancel(request.filepath);
    if (!cancelled) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    requests_.erase(id);
    (void)queue_.CancelPending(id);
    return true;
}

uint32_t RuntimeAssetStreamBridge::Pump(uint32_t maxRequests) noexcept {
    if (maxRequests == 0U) return 0U;

    std::vector<LoadedEvent> local;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const size_t count = std::min<size_t>(maxRequests, events_.size());
        local.reserve(count);
        local.insert(local.end(),
                     std::make_move_iterator(events_.begin()),
                     std::make_move_iterator(events_.begin() + static_cast<std::ptrdiff_t>(count)));
        events_.erase(events_.begin(),
                      events_.begin() + static_cast<std::ptrdiff_t>(count));
    }

    uint32_t processed = 0U;
    for (LoadedEvent& event : local) {
        ++processed;
        if (!event.success) {
            (void)queue_.FailUpload(event.id);
            std::lock_guard<std::mutex> lock(mutex_);
            requests_.erase(event.id);
            continue;
        }

        if (event.request.kind > static_cast<uint8_t>(AssetKind::Audio)) {
            (void)queue_.FailUpload(event.id);
            std::lock_guard<std::mutex> lock(mutex_);
            requests_.erase(event.id);
            continue;
        }
        const AssetKind kind = ToAssetKind(event.request.kind);
        const AssetDefinition* existing = assets_.Find(event.request.id);
        bool committed = false;
        if (existing != nullptr) {
            if (existing->kind == kind && existing->state == AssetState::Ready) {
                committed = assets_.ReplaceBytes(event.request.id, std::move(event.bytes));
            }
        } else {
            committed = assets_.ImportBytes(event.request.id, kind, {}, std::move(event.bytes)) &&
                        assets_.MarkReady(event.request.id);
        }
        if (!committed) {
            (void)queue_.FailUpload(event.id);
            std::lock_guard<std::mutex> lock(mutex_);
            requests_.erase(event.id);
            continue;
        }

        AssetResourceHandle handle{};
        if (!resources_.Acquire(event.request.id, handle) ||
            !resources_.BeginGpuUpload(handle)) {
            (void)queue_.FailUpload(event.id);
            if (handle.slot != 0xFFFFU) (void)resources_.Release(handle);
            std::lock_guard<std::mutex> lock(mutex_);
            requests_.erase(event.id);
            continue;
        }

        StreamRequest uploadRequest{};
        if (!queue_.BeginUpload(event.id, uploadRequest)) {
            (void)resources_.CancelGpuUpload(handle);
            (void)resources_.Release(handle);
            std::lock_guard<std::mutex> lock(mutex_);
            requests_.erase(event.id);
            continue;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        gpuUploads_[event.id] = handle;
    }
    return processed;
}

bool RuntimeAssetStreamBridge::GetGpuUpload(
    AssetID id, StreamRequest& request, AssetResourceHandle& handle) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto upload = gpuUploads_.find(id);
    const auto requestIt = requests_.find(id);
    if (upload == gpuUploads_.end() || requestIt == requests_.end()) return false;
    handle = upload->second;
    request = requestIt->second;
    return true;
}

bool RuntimeAssetStreamBridge::CompleteGpuUpload(
    AssetID id, VkDeviceMemory gpuMemory, uint32_t allocatedSizeMB,
    AssetStreamingQueue::GpuMemoryReleaseCallback releaseCallback) noexcept {
    if (gpuMemory == VK_NULL_HANDLE || allocatedSizeMB == 0U || !releaseCallback) return false;

    AssetResourceHandle handle{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = gpuUploads_.find(id);
        if (it == gpuUploads_.end()) return false;
        handle = it->second;
    }

    // Queue acceptance is first so an ownership failure cannot publish resource
    // residency without a corresponding GPU-memory owner.
    if (!queue_.CompleteUpload(id, gpuMemory, allocatedSizeMB, std::move(releaseCallback))) return false;
    if (!resources_.CompleteGpuUpload(handle)) {
        (void)queue_.Release(id);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    residentGpuUploads_[id] = handle;
    gpuUploads_.erase(id);
    requests_.erase(id);
    return true;
}

bool RuntimeAssetStreamBridge::FailGpuUpload(AssetID id) noexcept {
    AssetResourceHandle handle{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = gpuUploads_.find(id);
        if (it == gpuUploads_.end()) return false;
        handle = it->second;
    }
    if (!queue_.FailUpload(id) || !resources_.CancelGpuUpload(handle)) return false;
    if (!resources_.Release(handle)) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    gpuUploads_.erase(id);
    requests_.erase(id);
    return true;
}

bool RuntimeAssetStreamBridge::ReleaseGpuUpload(AssetID id) noexcept {
    AssetResourceHandle handle{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = residentGpuUploads_.find(id);
        if (it == residentGpuUploads_.end()) return false;
        handle = it->second;
    }
    if (!queue_.Release(id)) return false;
    if (!resources_.Release(handle)) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    residentGpuUploads_.erase(id);
    return true;
}

bool RuntimeAssetStreamBridge::ReleaseAllGpuUploads() noexcept {
    std::vector<AssetID> ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ids.reserve(residentGpuUploads_.size());
        for (const auto& [id, handle] : residentGpuUploads_) {
            (void)handle;
            ids.push_back(id);
        }
    }
    bool success = true;
    for (const AssetID& id : ids) {
        if (!ReleaseGpuUpload(id)) success = false;
    }
    return success && ResidentGpuUploadCount() == 0U;
}

uint32_t RuntimeAssetStreamBridge::PendingGpuUploadCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(gpuUploads_.size());
}

uint32_t RuntimeAssetStreamBridge::ResidentGpuUploadCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(residentGpuUploads_.size());
}

AssetKind RuntimeAssetStreamBridge::ToAssetKind(uint8_t kind) noexcept {
    switch (kind) {
        case static_cast<uint8_t>(AssetKind::Texture): return AssetKind::Texture;
        case static_cast<uint8_t>(AssetKind::Mesh): return AssetKind::Mesh;
        case static_cast<uint8_t>(AssetKind::Material): return AssetKind::Material;
        case static_cast<uint8_t>(AssetKind::Prefab): return AssetKind::Prefab;
        case static_cast<uint8_t>(AssetKind::Scene): return AssetKind::Scene;
        case static_cast<uint8_t>(AssetKind::Audio): return AssetKind::Audio;
        default: return AssetKind::Texture;
    }
}

bool RuntimeAssetStreamBridge::QueueLoadedEvent(LoadedEvent event) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(std::move(event));
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace NeoEngine
