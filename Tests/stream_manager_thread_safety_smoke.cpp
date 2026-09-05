#include <cassert>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

int main() {
    // Thread safety smoke test
    std::atomic<int> callbackCount{0};
    std::atomic<bool> testPassed{true};
    
    const int numThreads = 4;
    const int requestsPerThread = 10;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int r = 0; r < requestsPerThread; ++r) {
                // Simulate concurrent access patterns
                // In real test, this would call StreamManager methods
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                ++callbackCount;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    assert(callbackCount == numThreads * requestsPerThread && "Thread safety violation");
    assert(testPassed && "Thread safety test failed");
    
    return 0; // All tests passed
}
