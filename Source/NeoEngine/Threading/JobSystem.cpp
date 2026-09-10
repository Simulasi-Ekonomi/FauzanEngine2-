#include "JobSystem.h"

namespace NeoEngine {

void JobSystem::Initialize(size_t numThreads) {
    if (running_.load(std::memory_order_acquire)) return;
    if (numThreads == 0) numThreads = 1;

    numWorkers_ = numThreads;
    workers_.reserve(numWorkers_);
    for (size_t i = 0; i < numWorkers_; ++i) {
        auto worker = std::make_unique<WorkerState>();
        for (size_t slot = 0; slot < QUEUE_SIZE; ++slot) {
            worker->jobs[slot].sequence.store(slot, std::memory_order_relaxed);
        }
        workers_.push_back(std::move(worker));
    }

    running_.store(true, std::memory_order_release);
    for (size_t i = 0; i < numWorkers_; ++i) {
        workers_[i]->worker = std::thread([this, i] { WorkerLoop(i); });
    }
}

void JobSystem::Shutdown() {
    if (!running_.load(std::memory_order_acquire) && workers_.empty()) return;

    WaitForAll();
    running_.store(false, std::memory_order_release);
    for (auto& worker : workers_) {
        if (worker->worker.joinable()) worker->worker.join();
    }
    workers_.clear();
    numWorkers_ = 0;
}

bool JobSystem::TryPush(WorkerState& worker, Job&& job) {
    size_t position = worker.enqueuePos.load(std::memory_order_relaxed);
    for (;;) {
        JobSlot& slot = worker.jobs[position % QUEUE_SIZE];
        const size_t sequence = slot.sequence.load(std::memory_order_acquire);
        const std::intptr_t difference =
            static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position);

        if (difference == 0) {
            if (worker.enqueuePos.compare_exchange_weak(
                    position, position + 1, std::memory_order_relaxed)) {
                slot.job = std::move(job);
                slot.sequence.store(position + 1, std::memory_order_release);
                return true;
            }
        } else if (difference < 0) {
            return false;
        } else {
            position = worker.enqueuePos.load(std::memory_order_relaxed);
        }
    }
}

bool JobSystem::TryPop(WorkerState& worker, Job& job) {
    size_t position = worker.dequeuePos.load(std::memory_order_relaxed);
    for (;;) {
        JobSlot& slot = worker.jobs[position % QUEUE_SIZE];
        const size_t sequence = slot.sequence.load(std::memory_order_acquire);
        const std::intptr_t difference =
            static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position + 1);

        if (difference == 0) {
            if (worker.dequeuePos.compare_exchange_weak(
                    position, position + 1, std::memory_order_relaxed)) {
                job = std::move(slot.job);
                slot.sequence.store(position + QUEUE_SIZE, std::memory_order_release);
                return true;
            }
        } else if (difference < 0) {
            return false;
        } else {
            position = worker.dequeuePos.load(std::memory_order_relaxed);
        }
    }
}

void JobSystem::Execute(Job&& job) {
    if (!job) return;

    if (!running_.load(std::memory_order_acquire) || workers_.empty()) {
        job();
        return;
    }

    static std::atomic<size_t> roundRobin{0};
    totalJobs_.fetch_add(1, std::memory_order_acq_rel);

    const size_t start = roundRobin.fetch_add(1, std::memory_order_relaxed);
    for (size_t attempt = 0; attempt < numWorkers_; ++attempt) {
        const size_t index = (start + attempt) % numWorkers_;
        if (TryPush(*workers_[index], std::move(job))) return;
    }

    totalJobs_.fetch_sub(1, std::memory_order_acq_rel);
    job();
}

void JobSystem::ExecuteRaw(RawJob job, void* context) {
    if (!job) return;
    Execute([job, context] { job(context); });
}

void JobSystem::WaitForAll() {
    while (totalJobs_.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }
}

void JobSystem::WorkerLoop(size_t workerIndex) {
    while (running_.load(std::memory_order_acquire) ||
           totalJobs_.load(std::memory_order_acquire) > 0) {
        Job job;
        bool found = TryPop(*workers_[workerIndex], job);

        if (!found) {
            for (size_t i = 0; i < numWorkers_; ++i) {
                if (i == workerIndex) continue;
                if (TryPop(*workers_[i], job)) {
                    found = true;
                    break;
                }
            }
        }

        if (found && job) {
            activeJobs_.fetch_add(1, std::memory_order_acq_rel);
            try {
                job();
            } catch (...) {
                activeJobs_.fetch_sub(1, std::memory_order_acq_rel);
                totalJobs_.fetch_sub(1, std::memory_order_acq_rel);
                throw;
            }
            activeJobs_.fetch_sub(1, std::memory_order_acq_rel);
            totalJobs_.fetch_sub(1, std::memory_order_acq_rel);
            continue;
        }

        std::this_thread::yield();
    }
}

} // namespace NeoEngine
