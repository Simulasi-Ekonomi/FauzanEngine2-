#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <array>
#include <memory>
#include <functional>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace NeoEngine {

struct alignas(64) TaggedIndex {
    uint64_t value = 0;
    void Set(uint32_t pos, uint32_t tag) noexcept { value = (uint64_t(tag) << 32) | pos; }
    uint32_t Pos() const noexcept { return uint32_t(value & 0xFFFFFFFFU); }
    uint32_t Tag() const noexcept { return uint32_t(value >> 32); }
};

class JobSystem {
public:
    using Job = std::function<void()>;
    using RawJob = void (*)(void*);

    static JobSystem& Get() { static JobSystem instance; return instance; }

    void Initialize(size_t numThreads = std::thread::hardware_concurrency());
    void Shutdown();
    void Execute(Job&& job);
    void ExecuteRaw(RawJob job, void* context);
    void WaitForAll();
    size_t NumWorkers() const noexcept { return numWorkers_; }

private:
    static constexpr size_t QUEUE_SIZE = 256;

    struct alignas(64) JobSlot {
        std::atomic<size_t> sequence{0};
        Job job;
    };

    struct alignas(128) WorkerState {
        std::atomic<size_t> enqueuePos{0};
        std::atomic<size_t> dequeuePos{0};
        std::array<JobSlot, QUEUE_SIZE> jobs;
        std::thread worker;
    };

    void WorkerLoop(size_t workerIndex);
    bool TryPush(WorkerState& worker, Job&& job);
    bool TryPop(WorkerState& worker, Job& job);

    std::vector<std::unique_ptr<WorkerState>> workers_;
    std::atomic<bool> running_{false};
    std::atomic<int> activeJobs_{0};
    std::atomic<int> totalJobs_{0};
    size_t numWorkers_ = 0;
};

} // namespace NeoEngine
