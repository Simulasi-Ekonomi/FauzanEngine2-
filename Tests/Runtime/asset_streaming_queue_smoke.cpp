#include "AssetStreamingQueue.h"

#include <cassert>
#include <cstdint>
#include <iostream>
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

    assert(queue.CompleteUpload("high", static_cast<VkDeviceMemory>(1), 3));
    assert(queue.IsReady("high"));
    assert(queue.GetMemory("high") == static_cast<VkDeviceMemory>(1));
    assert(queue.GetResidentMB() == 3);

    assert(queue.TryDequeue(next));
    assert(next.id == "low");
    assert(queue.FailUpload("low"));
    assert(queue.GetState("low") == StreamState::Failed);
    assert(queue.GetAssetCount() == 1);

    assert(queue.Enqueue(StreamRequest{"old", "old.obj", 2.0f, 4, 1}));
    assert(queue.TryDequeue(next));
    assert(next.id == "old");
    assert(queue.CompleteUpload("old", static_cast<VkDeviceMemory>(2), 4));
    queue.MarkAccessed("high", 20);
    queue.MarkAccessed("old", 10);
    assert(queue.GetResidentMB() == 7);

    assert(queue.Enqueue(StreamRequest{"over", "over.obj", 1.0f, 1, 1}));
    assert(queue.TryDequeue(next));
    assert(next.id == "over");
    // Total resident budget, not just per-allocation budget, is the contract.
    assert(!queue.CompleteUpload("over", static_cast<VkDeviceMemory>(3), 2));
    assert(queue.GetState("over") == StreamState::Uploading);
    assert(queue.FailUpload("over"));

    queue.SetMemoryBudgetMB(4);
    assert(queue.EvictToBudget());
    assert(queue.GetResidentMB() <= 4);
    assert(queue.IsReady("high"));
    assert(!queue.IsReady("old"));
    assert(released.size() == 1);
    assert(released[0] == static_cast<VkDeviceMemory>(2));

    assert(queue.Release("high"));
    assert(queue.GetResidentMB() == 0);
    assert(!queue.Release("high"));
    assert(released.size() == 2);
    assert(released[1] == static_cast<VkDeviceMemory>(1));

    std::cout << "ASSET_STREAMING_QUEUE_SMOKE_OK\n";
    return 0;
}
