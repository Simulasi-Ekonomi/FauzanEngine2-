#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

namespace NeoEngine {

struct StreamRequest {
    std::string assetPath;
    int priority = 5;
    std::function<void(void*)> onLoaded;
};

struct StreamedAsset {
    std::string path;
    void* data = nullptr;
    size_t size = 0;
    bool loaded = false;
};

class StreamManager {
public:
    StreamManager(int maxWorkers = 2) : m_MaxWorkers(maxWorkers) {}
    ~StreamManager() { Stop(); }

    void Start() {
        m_Running = true;
        for (int i = 0; i < m_MaxWorkers; i++) {
            m_Workers.emplace_back(&StreamManager::WorkerLoop, this);
        }
    }

    void Stop() {
        m_Running = false;
        for (auto& w : m_Workers) {
            if (w.joinable()) w.join();
        }
        m_Workers.clear();
    }

    void RequestLoad(const std::string& path, int priority, std::function<void(void*)> callback) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Queue.push({path, priority, callback});
    }

    void* GetAsset(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_LoadedAssets.find(path);
        return it != m_LoadedAssets.end() ? it->second.data : nullptr;
    }

    bool IsLoaded(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_LoadedAssets.find(path);
        return it != m_LoadedAssets.end() && it->second.loaded;
    }

    void UnloadAsset(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_LoadedAssets.find(path);
        if (it != m_LoadedAssets.end()) {
            if (it->second.data) free(it->second.data);
            m_LoadedAssets.erase(it);
        }
    }

    size_t GetQueueSize() const { return m_Queue.size(); }
    size_t GetLoadedCount() const { return m_LoadedAssets.size(); }

private:
    void WorkerLoop() {
        while (m_Running) {
            StreamRequest req;
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                if (!m_Queue.empty()) {
                    req = m_Queue.front();
                    m_Queue.pop();
                }
            }
            if (!req.assetPath.empty()) {
                // Simulasi loading (ganti dengan file I/O sebenarnya)
                StreamedAsset asset;
                asset.path = req.assetPath;
                asset.data = malloc(1024); // placeholder
                asset.size = 1024;
                asset.loaded = true;
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    m_LoadedAssets[req.assetPath] = asset;
                }
                if (req.onLoaded) req.onLoaded(asset.data);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    std::queue<StreamRequest> m_Queue;
    std::unordered_map<std::string, StreamedAsset> m_LoadedAssets;
    std::vector<std::thread> m_Workers;
    std::mutex m_Mutex;
    std::atomic<bool> m_Running{false};
    int m_MaxWorkers;
};

} // namespace NeoEngine
