#include "AssetStreamingQueue.h"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace NeoEngine;

int main() {
    AssetStreamingQueue queue(8, 8);

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

    queue.SetMemoryBudgetMB(4);
    assert(queue.EvictToBudget());
    assert(queue.GetResidentMB() <= 4);
    assert(queue.IsReady("high"));
    assert(!queue.IsReady("old"));

    assert(queue.Release("high"));
    assert(queue.GetResidentMB() == 0);
    assert(!queue.Release("high"));

    std::cout << "ASSET_STREAMING_QUEUE_SMOKE_OK\n";
    return 0;
}
