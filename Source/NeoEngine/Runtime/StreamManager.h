#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace NeoEngine {

struct AsyncFileStreamRequest {
    std::string assetPath;
    int priority = 5;
    std::function<void(const std::vector<uint8_t>&)> onLoaded;
    std::function<void(bool)> onComplete;
};

struct StreamedAsset {
    std::string path;
    std::vector<uint8_t> data;
    size_t size = 0;
    bool loaded = false;
};

class StreamManager {
public:
    explicit StreamManager(int maxWorkers = 2, size_t maxQueuedRequests = 256,
                           size_t maxAssetBytes = 64U * 1024U * 1024U,
                           size_t maxResidentBytes = 256U * 1024U * 1024U) noexcept;
    ~StreamManager() noexcept;

    StreamManager(const StreamManager&) = delete;
    StreamManager& operator=(const StreamManager&) = delete;

    void Start() noexcept;
    void Stop() noexcept;
    [[nodiscard]] bool RequestLoad(const std::string& path, int priority,
                     std::function<void(const std::vector<uint8_t>&)> callback,
                     std::function<void(bool)> onComplete = {}) noexcept;
    // Cancels a queued or active file read. Active reads observe cancellation
    // between bounded chunks; the data callback is suppressed for cancelled loads,
    // while the completion callback still receives false exactly once.
    [[nodiscard]] bool Cancel(const std::string& path) noexcept;

    // Compatibility accessor: the pointer remains valid only until that asset is
    // unloaded/replaced or the manager is destroyed. Concurrent callers should
    // use GetAssetCopy, which takes a stable snapshot under the manager lock.
    [[deprecated("Use GetAssetCopy for concurrent access")]]
    [[nodiscard]] const std::vector<uint8_t>* GetAsset(const std::string& path) const noexcept;
    [[nodiscard]] bool GetAssetCopy(const std::string& path, std::vector<uint8_t>& out) const noexcept;
    [[nodiscard]] bool IsLoaded(const std::string& path) const noexcept;
    void UnloadAsset(const std::string& path) noexcept;

    [[nodiscard]] size_t GetQueueSize() const noexcept;
    [[nodiscard]] size_t GetLoadedCount() const noexcept;
    [[nodiscard]] size_t GetResidentBytes() const noexcept;

private:
    struct CancellationState { std::atomic<bool> cancelled{false}; };
    struct QueuedRequest {
        AsyncFileStreamRequest request;
        uint64_t sequence = 0;
        std::shared_ptr<CancellationState> cancellation;
    };
    void WorkerLoop() noexcept;
    [[nodiscard]] bool LoadAssetFile(const std::string& path,
                                     const std::shared_ptr<CancellationState>& cancellation,
                                     std::vector<uint8_t>& data) const noexcept;

    std::vector<QueuedRequest> m_Queue;
    std::unordered_map<std::string, StreamedAsset> m_LoadedAssets;
    std::unordered_map<std::string, std::shared_ptr<CancellationState>> m_Requests;
    std::vector<std::thread> m_Workers;
    mutable std::mutex m_Mutex;
    std::condition_variable m_Condition;
    bool m_Running = false;
    uint64_t m_NextSequence = 1;
    size_t m_WorkerCount;
    size_t m_QueuedLimit;
    size_t m_MaxAssetBytes;
    size_t m_MaxResidentBytes;
    size_t m_ResidentBytes = 0;
};

} // namespace NeoEngine
