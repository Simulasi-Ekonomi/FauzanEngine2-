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

bool RuntimeAssetStreamBridge::Refresh(const StreamRequest& request) noexcept {
    if (request.id.empty() || request.filepath.empty() || request.estimatedSizeMB == 0U) return false;

    AssetResourceHandle handle{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_ || requests_.contains(request.id)) return false;
        const auto resident = residentGpuUploads_.find(request.id);
        if (resident == residentGpuUploads_.end()) return false;
        handle = resident->second;

        // Begin the refresh while the resident resource remains leased and GPU-resident.
        // The queue and resource manager both retain the previous owner until completion.
        if (!resources_.BeginGpuRefresh(handle)) return false;
        if (!queue_.BeginRefresh(request)) {
            (void)resources_.CancelGpuRefresh(handle);
            return false;
        }
        try {
            gpuUploads_.emplace(request.id, handle);
            requests_.emplace(request.id, request);
        } catch (...) {
            (void)queue_.FailUpload(request.id);
            (void)resources_.CancelGpuRefresh(handle);
            gpuUploads_.erase(request.id);
            requests_.erase(request.id);
            return false;
        }
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
    if (accepted) return true;

    (void)queue_.FailUpload(request.id);
    (void)resources_.CancelGpuRefresh(handle);
    std::lock_guard<std::mutex> lock(mutex_);
    gpuUploads_.erase(request.id);
    requests_.erase(request.id);
    return false;
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
        AssetResourceHandle pendingHandle{};
        bool isRefresh = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto resident = residentGpuUploads_.find(event.id);
            const auto pending = gpuUploads_.find(event.id);
            if (resident != residentGpuUploads_.end() && pending != gpuUploads_.end() &&
                resident->second == pending->second) {
                isRefresh = true;
                pendingHandle = pending->second;
            }
        }

        const auto failEvent = [this, &event, isRefresh, pendingHandle]() noexcept {
            if (isRefresh) {
                (void)queue_.FailUpload(event.id);
                (void)resources_.CancelGpuRefresh(pendingHandle);
            } else {
                (void)queue_.CancelPending(event.id);
            }
            std::lock_guard<std::mutex> lock(mutex_);
            gpuTexturePayloads_.erase(event.id);
            pendingRefreshBytes_.erase(event.id);
            gpuUploads_.erase(event.id);
            requests_.erase(event.id);
        };

        if (!event.success) {
            failEvent();
            continue;
        }

        if (event.request.kind > static_cast<uint8_t>(AssetKind::Audio)) {
            failEvent();
            continue;
        }
        const AssetKind kind = ToAssetKind(event.request.kind);
        GpuTexturePayload decodedTexture{};
        if (kind == AssetKind::Texture) {
            RgbaTexture decoded{};
            TextureDecodeError decodeError = TextureDecodeError::None;
            bool decodedOk = PpmTextureDecoder::DecodeP6(event.bytes, decoded, decodeError);
            if (!decodedOk) decodedOk = BmpTextureDecoder::DecodeBiRgb(event.bytes, decoded, decodeError);
            if (!decodedOk || decoded.width == 0U || decoded.height == 0U || decoded.rgba.empty()) {
                (void)queue_.FailUpload(event.id);
                std::lock_guard<std::mutex> lock(mutex_);
                requests_.erase(event.id);
                return processed;
            }
            decodedTexture.rgba = std::move(decoded.rgba);
            decodedTexture.width = decoded.width;
            decodedTexture.height = decoded.height;
        }
        const AssetDefinition* existing = assets_.Find(event.request.id);
        bool committed = false;
        if (isRefresh) {
            // Keep the old registry definition authoritative until the replacement
            // GPU upload has completed. This prevents an in-use resource from being
            // marked stale while its old GPU representation is still active.
            try {
                std::lock_guard<std::mutex> lock(mutex_);
                pendingRefreshBytes_[event.id] = std::move(event.bytes);
                if (kind == AssetKind::Texture) gpuTexturePayloads_[event.id] = std::move(decodedTexture);
            } catch (...) {
                failEvent();
                continue;
            }
            committed = true;
        } else if (existing != nullptr) {
            if (existing->kind == kind && existing->state == AssetState::Ready) {
                committed = assets_.ReplaceBytes(event.request.id, std::move(event.bytes));
            }
        } else {
            committed = assets_.ImportBytes(event.request.id, kind, {}, std::move(event.bytes)) &&
                        assets_.MarkReady(event.request.id);
        }
        if (!committed) {
            failEvent();
            continue;
        }

        AssetResourceHandle handle{};
        if (isRefresh) {
            handle = pendingHandle;
        } else {
            if (!resources_.Acquire(event.request.id, handle) ||
                !resources_.BeginGpuUpload(handle)) {
                (void)queue_.FailUpload(event.id);
                if (handle.slot != 0xFFFFU) (void)resources_.Release(handle);
                std::lock_guard<std::mutex> lock(mutex_);
                requests_.erase(event.id);
                continue;
            }
        }

        if (!isRefresh) {
            StreamRequest uploadRequest{};
            if (!queue_.BeginUpload(event.id, uploadRequest)) {
                (void)resources_.CancelGpuUpload(handle);
                (void)resources_.Release(handle);
                std::lock_guard<std::mutex> lock(mutex_);
                requests_.erase(event.id);
                continue;
            }
        }

        if (!isRefresh && kind == AssetKind::Texture) {
            std::lock_guard<std::mutex> lock(mutex_);
            gpuTexturePayloads_[event.id] = std::move(decodedTexture);
        }
    }
    return processed;
}

bool RuntimeAssetStreamBridge::GetGpuUploadTextureData(
    AssetID id, std::vector<uint8_t>& rgba, uint32_t& width, uint32_t& height) const noexcept {
    rgba.clear();
    width = 0U;
    height = 0U;
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = gpuTexturePayloads_.find(id);
    if (it == gpuTexturePayloads_.end() || it->second.rgba.empty() ||
        it->second.width == 0U || it->second.height == 0U) return false;
    rgba = it->second.rgba;
    width = it->second.width;
    height = it->second.height;
    return true;
}

bool RuntimeAssetStreamBridge::GetPendingGpuUploadIds(std::vector<AssetID>& ids) const noexcept {
    ids.clear();
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        ids.reserve(gpuUploads_.size());
        for (const auto& [id, handle] : gpuUploads_) {
            (void)handle;
            ids.push_back(id);
        }
        return true;
    } catch (...) {
        ids.clear();
        return false;
    }
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
    gpuTexturePayloads_.erase(id);
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
    gpuTexturePayloads_.erase(id);
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
    gpuTexturePayloads_.erase(id);
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
