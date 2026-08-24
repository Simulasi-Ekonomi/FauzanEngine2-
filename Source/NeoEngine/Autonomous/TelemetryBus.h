#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <any>
#include <chrono>
#include <functional>
#include <mutex>

namespace NeoEngine {

struct TelemetrySnapshot {
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::any> metrics;
    int frameNumber = 0;
    float deltaTime = 0.0f;
    float fps = 0.0f;
    size_t memoryUsage = 0;
    int actorCount = 0;
    int entityCount = 0;
};

class TelemetryBus {
public:
    void Initialize() { m_Initialized = true; }

    void Collect() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Snapshot.timestamp = std::chrono::system_clock::now();
        m_Snapshot.frameNumber++;
        // Metrik akan diisi oleh sistem eksternal melalui PublishMetric
    }

    TelemetrySnapshot GetSnapshot() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Snapshot;
    }

    template<typename T>
    void PublishMetric(const std::string& key, T value) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Snapshot.metrics[key] = std::any(value);
        if (m_Subscribers.count(key)) {
            for (auto& cb : m_Subscribers[key]) {
                cb(std::any(value));
            }
        }
    }

    void Subscribe(const std::string& key, std::function<void(const std::any&)> callback) {
        m_Subscribers[key].push_back(callback);
    }

private:
    TelemetrySnapshot m_Snapshot;
    std::unordered_map<std::string, std::vector<std::function<void(const std::any&)>>> m_Subscribers;
    mutable std::mutex m_Mutex;
    bool m_Initialized = false;
};

} // namespace NeoEngine
