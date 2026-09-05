#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <unordered_map>

namespace NeoEngine {

struct StreamRequest {
    std::string assetPath;
    int priority = 5;
    std::function<void(const std::vector<uint8_t>&)> onLoaded;
};

struct StreamedAsset {
    std::string path;
    std::vector<uint8_t> data;
    size_t size = 0;
    bool loaded = false;
};

class StreamManager {
public:
    explicit StreamManager(int maxWorkers = 2) noexcept : m_MaxWorkers(maxWorkers) {}
    ~StreamManager() noexcept { Stop(); }

    StreamManager(const StreamManager&) = delete;
    StreamManager& operator=(const StreamManager&) = delete;

    void Start() noexcept;
    void Stop() noexcept;
    void RequestLoad(const std::string& path, int priority, 
                     std::function<void(const std::vector<uint8_t>&)> callback) noexcept;
    
    [[nodiscard]] const std::vector<uint8_t>* GetAsset(const std::string& path) const noexcept;
    [[nodiscard]] bool IsLoaded(const std::string& path) const noexcept;
    void UnloadAsset(const std::string& path) noexcept;

    [[nodiscard]] size_t GetQueueSize() const noexcept { return m_Queue.size(); }
    [[nodiscard]] size_t GetLoadedCount() const noexcept { return m_LoadedAssets.size(); }

private:
    void WorkerLoop() noexcept;
    [[nodiscard]] std::vector<uint8_t> LoadAssetFile(const std::string& path) const noexcept;

    std::queue<StreamRequest> m_Queue;
    mutable std::unordered_map<std::string, StreamedAsset> m_LoadedAssets;
    std::vector<std::thread> m_Workers;
    mutable std::mutex m_Mutex;
    std::atomic<bool> m_Running{false};
    int m_MaxWorkers;
};

} // namespace NeoEngine
