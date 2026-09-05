#include "StreamManager.h"
#include <fstream>
#include <algorithm>
#include <cstring>

namespace NeoEngine {

void StreamManager::Start() noexcept {
    m_Running = true;
    for (int i = 0; i < m_MaxWorkers; ++i) {
        m_Workers.emplace_back(&StreamManager::WorkerLoop, this);
    }
}

void StreamManager::Stop() noexcept {
    m_Running = false;
    for (auto& worker : m_Workers) {
        if (worker.joinable()) worker.join();
    }
    m_Workers.clear();
}

void StreamManager::RequestLoad(const std::string& path, int priority, 
                                 std::function<void(const std::vector<uint8_t>&)> callback) noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Queue.push({path, priority, callback});
}

const std::vector<uint8_t>* StreamManager::GetAsset(const std::string& path) const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_LoadedAssets.find(path);
    return (it != m_LoadedAssets.end() && it->second.loaded) ? &it->second.data : nullptr;
}

bool StreamManager::IsLoaded(const std::string& path) const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_LoadedAssets.find(path);
    return it != m_LoadedAssets.end() && it->second.loaded;
}

void StreamManager::UnloadAsset(const std::string& path) noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_LoadedAssets.erase(path);
}

std::vector<uint8_t> StreamManager::LoadAssetFile(const std::string& path) const noexcept {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::vector<uint8_t> data;
    
    if (!file.is_open()) return data;
    
    std::streamsize size = file.tellg();
    if (size <= 0) return data;
    
    file.seekg(0, std::ios::beg);
    
    try {
        data.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), size);
    } catch (...) {
        data.clear();
    }
    
    return data;
}

void StreamManager::WorkerLoop() noexcept {
    while (m_Running) {
        StreamRequest req;
        bool hasRequest = false;
        
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (!m_Queue.empty()) {
                req = m_Queue.front();
                m_Queue.pop();
                hasRequest = true;
            }
        }
        
        if (hasRequest && !req.assetPath.empty()) {
            std::vector<uint8_t> fileData = LoadAssetFile(req.assetPath);
            
            if (!fileData.empty()) {
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    m_LoadedAssets[req.assetPath] = {req.assetPath, fileData, fileData.size(), true};
                }
                if (req.onLoaded) req.onLoaded(fileData);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace NeoEngine
