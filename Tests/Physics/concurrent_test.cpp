#include "Threading/JobSystem.h"
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

using namespace NeoEngine;

int main() {
    constexpr size_t workerCount = 8U;
    constexpr size_t jobsPerWorker = 4096U;
    constexpr size_t totalJobs = workerCount * jobsPerWorker;

    JobSystem& jobs = JobSystem::Get();
    jobs.Initialize(workerCount);

    std::atomic<size_t> completed{0U};
    std::atomic<uint64_t> checksum{0U};

    for (size_t worker = 0U; worker < workerCount; ++worker) {
        for (size_t jobIndex = 0U; jobIndex < jobsPerWorker; ++jobIndex) {
            const uint64_t token = (static_cast<uint64_t>(worker) << 32U) | jobIndex;
            jobs.Execute([&completed, &checksum, token]() {
                checksum.fetch_add((token ^ 0x9E3779B97F4A7C15ULL) + 1ULL, std::memory_order_relaxed);
                completed.fetch_add(1U, std::memory_order_relaxed);
            });
        }
    }

    jobs.WaitForAll();

    const uint64_t expected = [] {
        uint64_t value = 0U;
        for (size_t worker = 0U; worker < workerCount; ++worker) {
            for (size_t jobIndex = 0U; jobIndex < jobsPerWorker; ++jobIndex) {
                const uint64_t token = (static_cast<uint64_t>(worker) << 32U) | jobIndex;
                value += (token ^ 0x9E3779B97F4A7C15ULL) + 1ULL;
            }
        }
        return value;
    }();

    const bool pass = completed.load(std::memory_order_acquire) == totalJobs &&
                      checksum.load(std::memory_order_acquire) == expected;
    std::printf("JobSystem concurrent stress: jobs=%zu completed=%zu checksum=%s\n",
                totalJobs, completed.load(), pass ? "PASS" : "FAIL");

    jobs.Shutdown();
    return pass ? 0 : 1;
}
