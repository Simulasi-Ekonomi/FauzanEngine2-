#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <atomic>

namespace NeoEngine {

using Job = std::function<void()>;

class JobSystem {
public:
    void Start(size_t numThreads = 2) {
        running_ = true;
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back(&JobSystem::WorkerLoop, this);
        }
    }

    void Shutdown() {
        running_ = false;
        cv_.notify_all();
        for (auto& w : workers_) if (w.joinable()) w.join();
    }

    void Schedule(Job job) {
        std::lock_guard<std::mutex> lock(mtx_);
        jobs_.push(job);
        cv_.notify_one();
    }

private:
    void WorkerLoop() {
        while (running_) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [this]{ return !running_ || !jobs_.empty(); });
                if (!running_ && jobs_.empty()) break;
                job = jobs_.front(); jobs_.pop();
            }
            job();
        }
    }

    std::atomic<bool> running_{false};
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<Job> jobs_;
    std::vector<std::thread> workers_;
};

} // namespace
