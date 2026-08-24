#pragma once
#include "RLAgent.h"
#include "RLSimulator.h"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

namespace NeoEngine {

// Data Buffer (asinkron, decoupled training & rollout) – inspirasi slime/GLM-5
template<typename T>
class DataBuffer {
public:
    void Push(const T& item) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Buffer.push(item);
        m_CV.notify_one();
    }

    T Pop() {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_CV.wait(lock, [this] { return !m_Buffer.empty(); });
        T item = m_Buffer.front();
        m_Buffer.pop();
        return item;
    }

    bool Empty() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Buffer.empty();
    }

    size_t Size() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Buffer.size();
    }

private:
    std::queue<T> m_Buffer;
    std::mutex m_Mutex;
    std::condition_variable m_CV;
};

struct RolloutExperience {
    std::vector<float> state;
    int action;
    float reward;
    std::vector<float> nextState;
};

class RLTrainingPipeline {
public:
    RLTrainingPipeline() : m_Running(false) {}

    void Start(int numWorkers = 2) {
        m_Running = true;
        for (int i = 0; i < numWorkers; ++i) {
            m_Workers.emplace_back(&RLTrainingPipeline::RolloutWorker, this);
        }
    }

    void Stop() {
        m_Running = false;
        m_DataBuffer.Push(RolloutExperience{}); // wake up workers
        for (auto& w : m_Workers) if (w.joinable()) w.join();
        m_Workers.clear();
    }

    void ProvideExperience(const RolloutExperience& exp) {
        m_DataBuffer.Push(exp);
    }

    RolloutExperience GetExperience() {
        return m_DataBuffer.Pop();
    }

    size_t GetBufferSize() { return m_DataBuffer.Size(); }

private:
    DataBuffer<RolloutExperience> m_DataBuffer;
    std::vector<std::thread> m_Workers;
    std::atomic<bool> m_Running;

    void RolloutWorker() {
        while (m_Running) {
            RolloutExperience exp = GetExperience();
            // Proses data latihan (placeholder)
            if (!m_Running) break;
        }
    }
};

} // namespace NeoEngine
