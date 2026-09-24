#include "AssetStreamingQueue.h"

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace {
template <typename T>
T FakeDeviceMemory(uintptr_t value) {
    if constexpr (std::is_pointer_v<VkDeviceMemory>) {
        return reinterpret_cast<T>(value);
    } else {
        return static_cast<T>(value);
    }
}
}
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

using namespace NeoEngine;

int main() {
    AssetStreamingQueue queue(8, 8);
    std::vector<VkDeviceMemory> released;
    queue.SetGpuMemoryReleaseCallback([&released](VkDeviceMemory memory) { released.push_back(memory); });
    assert(queue.HasGpuMemoryReleaseCallback());

    StreamRequest low{"low", "low.obj", 1.0f, 2, 1};
    StreamRequest high{"high", "high.obj", 10.0f, 3, 1};
    StreamRequest invalid{"invalid", "", 1.0f, 1, 1};

    assert(queue.Enqueue(low));
    assert(queue.Enqueue(high));
    assert(!queue.Enqueue(invalid));
    assert(queue.GetQueuedCount() == 2);
    assert(queue.GetAssetCount() == 2);

    StreamRequest next{};
    assert(queue.TryDequeue(next));
    assert(next.id == "high");
    assert(queue.GetState("high") == StreamState::Uploading);
    assert(!queue.IsReady("high"));

    assert(queue.CompleteUpload("high", FakeDeviceMemory<VkDeviceMemory>(1), 3));
    assert(queue.IsReady("high"));
    assert(queue.GetMemory("high") == FakeDeviceMemory(1));
    assert(queue.GetResidentMB() == 3);

    assert(queue.TryDequeue(next));
    assert(next.id == "low");
    assert(queue.FailUpload("low"));
    assert(queue.GetState("low") == StreamState::Failed);
    assert(queue.GetAssetCount() == 1);

    assert(queue.Enqueue(StreamRequest{"old", "old.obj", 2.0f, 4, 1}));
    assert(queue.TryDequeue(next));
    assert(next.id == "old");
    assert(queue.CompleteUpload("old", FakeDeviceMemory<VkDeviceMemory>(2), 4));
    queue.MarkAccessed("high", 20);
    queue.MarkAccessed("old", 10);
    assert(queue.GetResidentMB() == 7);

    assert(queue.Enqueue(StreamRequest{"over", "over.obj", 1.0f, 1, 1}));
    assert(queue.TryDequeue(next));
    assert(next.id == "over");
    // Total resident budget, not just per-allocation budget, is the contract.
    assert(!queue.CompleteUpload("over", FakeDeviceMemory<VkDeviceMemory>(3), 2));
    assert(queue.GetState("over") == StreamState::Uploading);
    assert(queue.FailUpload("over"));

    queue.SetMemoryBudgetMB(4);
    assert(queue.EvictToBudget());
    assert(queue.GetResidentMB() <= 4);
    assert(queue.IsReady("high"));
    assert(!queue.IsReady("old"));
    assert(released.size() == 1);
    assert(released[0] == FakeDeviceMemory(2));

    assert(queue.Release("high"));
    assert(queue.GetResidentMB() == 0);
    assert(!queue.Release("high"));
    assert(released.size() == 2);
    assert(released[1] == FakeDeviceMemory(1));

    // Existing allocations retain the releaser that owned them even if the
    // queue callback is replaced later (e.g. after a Vulkan device recreation).
    AssetStreamingQueue ownershipQueue(8, 8);
    std::vector<VkDeviceMemory> ownerA;
    std::vector<VkDeviceMemory> ownerB;
    ownershipQueue.SetGpuMemoryReleaseCallback([&ownerA](VkDeviceMemory memory) { ownerA.push_back(memory); });
    assert(ownershipQueue.Enqueue(StreamRequest{"owned", "owned.obj", 1.0f, 2, 1}));
    assert(ownershipQueue.TryDequeue(next));
    assert(next.id == "owned");
    assert(ownershipQueue.CompleteUpload("owned", FakeDeviceMemory(11), 2));
    ownershipQueue.SetGpuMemoryReleaseCallback([&ownerB](VkDeviceMemory memory) { ownerB.push_back(memory); });
    assert(ownershipQueue.Release("owned"));
    assert(ownerA.size() == 1 && ownerA[0] == FakeDeviceMemory(11));
    assert(ownerB.empty());

    // A throwing releaser must not make an allocation disappear or corrupt
    // resident accounting. A shared state lets the test recover and retry.
    AssetStreamingQueue retryQueue(8, 8);
    auto throwOnce = std::make_shared<bool>(true);
    std::vector<VkDeviceMemory> retryReleased;
    retryQueue.SetGpuMemoryReleaseCallback([throwOnce, &retryReleased](VkDeviceMemory memory) {
        if (*throwOnce) { *throwOnce = false; throw 1; }
        retryReleased.push_back(memory);
    });
    assert(retryQueue.Enqueue(StreamRequest{"retry", "retry.obj", 1.0f, 2, 1}));
    assert(retryQueue.TryDequeue(next));
    assert(retryQueue.CompleteUpload("retry", FakeDeviceMemory(12), 2));
    assert(!retryQueue.Release("retry"));
    assert(retryQueue.IsReady("retry"));
    assert(retryQueue.GetMemory("retry") == FakeDeviceMemory(12));
    assert(retryQueue.GetResidentMB() == 2);
    assert(retryQueue.Release("retry"));
    assert(retryReleased.size() == 1 && retryReleased[0] == FakeDeviceMemory(12));
    assert(retryQueue.GetResidentMB() == 0);

    std::cout << "ASSET_STREAMING_QUEUE_SMOKE_OK\n";
    return 0;
}
