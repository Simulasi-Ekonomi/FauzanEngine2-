#include "JobSystem.h"

namespace NeoEngine {

static inline TaggedIndex LoadTagged(const std::atomic<uint64_t>& src) {
    TaggedIndex ti; ti.value = src.load(std::memory_order_acquire); return ti;
}
static inline void StoreTagged(std::atomic<uint64_t>& dst, TaggedIndex ti) {
    dst.store(ti.value, std::memory_order_release);
}

void JobSystem::Initialize(size_t numThreads) {
    if (running_) return;
    numWorkers_ = numThreads;
    running_ = true;
    workers_.reserve(numThreads);
    for (size_t i = 0; i < numWorkers_; ++i) {
        auto ws = std::make_unique<WorkerState>();
        StoreTagged(ws->head, {0}); StoreTagged(ws->tail, {0});
        workers_.push_back(std::move(ws));
    }
    for (size_t i = 0; i < numWorkers_; ++i) {
        workers_[i]->worker = std::thread([this, i] { WorkerLoop(i); });
    }
}

void JobSystem::Shutdown() {
    running_ = false;
    for (auto& ws : workers_) if (ws->worker.joinable()) ws->worker.join();
    workers_.clear();
}

void JobSystem::Execute(Job&& job) {
    static std::atomic<size_t> rr{0};
    for (size_t attempt = 0; attempt < numWorkers_; ++attempt) {
        size_t idx = rr.fetch_add(1, std::memory_order_relaxed) % numWorkers_;
        auto& ws = *workers_[idx];
        TaggedIndex tail = LoadTagged(ws.tail);
        TaggedIndex head = LoadTagged(ws.head);
        if (tail.Pos() - head.Pos() < QUEUE_SIZE) {
            ws.jobs[tail.Pos() % QUEUE_SIZE] = std::move(job);
            std::atomic_thread_fence(std::memory_order_release);
            tail.Set(tail.Pos() + 1, tail.Tag() + 1);
            StoreTagged(ws.tail, tail);
            totalJobs_.fetch_add(1, std::memory_order_release);
            return;
        }
    }
    job(); // fallback langsung
}

void JobSystem::ExecuteRaw(RawJob job, void* context) {
    if (!job) return;
    Execute([job, context]() { job(context); });
}

void JobSystem::WaitForAll() {
    while (totalJobs_.load(std::memory_order_acquire) > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void JobSystem::WorkerLoop(size_t workerIndex) {
    auto& ws = *workers_[workerIndex];
    while (running_.load(std::memory_order_acquire)) {
        TaggedIndex head = LoadTagged(ws.head);
        TaggedIndex tail = LoadTagged(ws.tail);
        if (head.Pos() < tail.Pos()) {
            TaggedIndex newHead;
            newHead.Set(head.Pos() + 1, head.Tag() + 1);
            if (std::atomic_compare_exchange_strong(
                    reinterpret_cast<std::atomic<uint64_t>*>(&ws.head),
                    &head.value, newHead.value)) {
                Job job = std::move(ws.jobs[head.Pos() % QUEUE_SIZE]);
                if (job) {
                    activeJobs_.fetch_add(1, std::memory_order_release);
                    job();
                    activeJobs_.fetch_sub(1, std::memory_order_release);
                    totalJobs_.fetch_sub(1, std::memory_order_release);
                    continue;
                }
            }
        }
        // Steal dari worker lain
        Job job;
        for (size_t i = 0; i < numWorkers_; ++i) {
            if (i == workerIndex) continue;
            if (TrySteal(*workers_[i], job)) break;
        }
        if (job) {
            activeJobs_.fetch_add(1, std::memory_order_release);
            job();
            activeJobs_.fetch_sub(1, std::memory_order_release);
            totalJobs_.fetch_sub(1, std::memory_order_release);
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    }
}

bool JobSystem::TrySteal(WorkerState& from, Job& job) {
    while (true) {
        TaggedIndex head = LoadTagged(from.head);
        TaggedIndex tail = LoadTagged(from.tail);
        if (head.Pos() >= tail.Pos()) return false;
        TaggedIndex newHead;
        newHead.Set(head.Pos() + 1, head.Tag() + 1);
        if (std::atomic_compare_exchange_strong(
                reinterpret_cast<std::atomic<uint64_t>*>(&from.head),
                &head.value, newHead.value)) {
            job = std::move(from.jobs[head.Pos() % QUEUE_SIZE]);
            return true;
        }
    }
}

} // namespace
