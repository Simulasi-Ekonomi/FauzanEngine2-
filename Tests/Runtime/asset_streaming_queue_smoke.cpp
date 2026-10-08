#include "AssetStreamingQueue.h"

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace {
template <typename Handle>
Handle FakeHandle(uintptr_t value) {
    if constexpr (std::is_pointer_v<Handle>) {
        return reinterpret_cast<Handle>(value);
    } else {
        return static_cast<Handle>(value);
    }
}

VkDeviceMemory FakeDeviceMemory(uintptr_t value) {
    return FakeHandle<VkDeviceMemory>(value);
}
}
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

using namespace NeoEngine;

#define CHECK(expr) do { if (!(expr)) return 1; } while (false)

int main() {
    AssetStreamingQueue queue(8, 8);
    std::vector<VkDeviceMemory> released;
    queue.SetGpuMemoryReleaseCallback([&released](VkDeviceMemory memory) { released.push_back(memory); });
    CHECK(queue.HasGpuMemoryReleaseCallback());

    StreamRequest low{"low", "low.obj", 1.0f, 2, 1};
    StreamRequest high{"high", "high.obj", 10.0f, 3, 1};
    StreamRequest invalid{"invalid", "", 1.0f, 1, 1};

    CHECK(queue.Enqueue(low));
    CHECK(queue.Enqueue(high));
    CHECK(!queue.Enqueue(invalid));
    CHECK(queue.GetAssetCount() == 2);

    CHECK(!queue.CancelPending(""));
    CHECK(queue.CancelPending("low"));
    CHECK(!queue.CancelPending("low"));
    CHECK(queue.GetQueuedCount() == 1);
    CHECK(queue.GetAssetCount() == 1);
    CHECK(queue.Enqueue(low));

    StreamRequest next{};
    // File I/O completion can arrive out of priority order. The targeted handoff
    // must transition that asset without stealing a different queued request.
    StreamRequest outOfOrder{};
    CHECK(queue.BeginUpload("low", outOfOrder));
    CHECK(outOfOrder.id == "low");
    CHECK(queue.GetState("low") == StreamState::Uploading);
    CHECK(queue.GetQueuedCount() == 1);
    CHECK(queue.GetState("high") == StreamState::Pending);
    CHECK(queue.FailUpload("low"));
    CHECK(queue.GetQueuedCount() == 1);
    // GPU ownership must be explicit; an upload without a release owner is rejected.
    AssetStreamingQueue ownershipRequiredQueue(8, 8);
    CHECK(ownershipRequiredQueue.Enqueue(StreamRequest{"unowned", "unowned.obj", 1.0f, 1, 1}));
    CHECK(ownershipRequiredQueue.TryDequeue(next));
    CHECK(!ownershipRequiredQueue.CompleteUpload("unowned", FakeDeviceMemory(99), 1));
    CHECK(ownershipRequiredQueue.GetState("unowned") == StreamState::Uploading);
    CHECK(ownershipRequiredQueue.FailUpload("unowned"));
    CHECK(queue.TryDequeue(next));
    CHECK(next.id == "high");
    CHECK(queue.GetState("high") == StreamState::Uploading);
    CHECK(!queue.IsReady("high"));
    CHECK(!queue.CancelPending("high"));

    CHECK(queue.CompleteUpload("high", FakeDeviceMemory(1), 3));
    CHECK(queue.IsReady("high"));
    CHECK(queue.GetMemory("high") == FakeDeviceMemory(1));
    CHECK(queue.GetResidentMB() == 3);

    AssetStreamingQueue refreshQueue(8, 8);
    std::vector<VkDeviceMemory> refreshReleased;
    refreshQueue.SetGpuMemoryReleaseCallback([&refreshReleased](VkDeviceMemory memory) {
        refreshReleased.push_back(memory);
    });
    CHECK(refreshQueue.Enqueue(StreamRequest{"refresh", "refresh.obj", 2.0f, 2, 1}));
    CHECK(refreshQueue.TryDequeue(next));
    CHECK(next.id == "refresh");
    CHECK(refreshQueue.CompleteUpload("refresh", FakeDeviceMemory(21), 2));
    CHECK(refreshQueue.BeginRefresh(StreamRequest{"refresh", "refresh-new.obj", 5.0f, 3, 1}));
    CHECK(refreshQueue.IsRefreshing("refresh"));
    CHECK(refreshQueue.GetResidentMB() == 2);
    CHECK(!refreshQueue.Release("refresh"));

    AssetStreamingQueue::GpuMemoryReleaseCallback oldRefreshOwner;
    VkDeviceMemory oldRefreshMemory = VK_NULL_HANDLE;
    CHECK(refreshQueue.IsRefreshing("refresh"));
    if (!refreshQueue.CompleteRefreshUpload(
            "refresh", FakeDeviceMemory(22), 3,
            [&refreshReleased](VkDeviceMemory memory) { refreshReleased.push_back(memory); },
            oldRefreshOwner, oldRefreshMemory)) {
        std::cerr << "REFRESH_COMPLETE_FAILED\\n";
        return 1;
    }
    CHECK(refreshQueue.IsReady("refresh"));
    CHECK(refreshQueue.GetMemory("refresh") == FakeDeviceMemory(22));
    CHECK(refreshQueue.GetResidentMB() == 3);
    CHECK(oldRefreshMemory == FakeDeviceMemory(21));
    CHECK(refreshReleased.empty());
    if (!oldRefreshOwner) {
        std::cerr << "REFRESH_OWNER_MISSING\n";
        return 1;
    }
    oldRefreshOwner(oldRefreshMemory);
    CHECK(refreshReleased.size() == 1 && refreshReleased[0] == FakeDeviceMemory(21));

    CHECK(refreshQueue.BeginRefresh(StreamRequest{"refresh", "refresh-failed.obj", 4.0f, 3, 1}));
    CHECK(refreshQueue.IsRefreshing("refresh"));
    CHECK(refreshQueue.FailUpload("refresh"));
    CHECK(refreshQueue.IsReady("refresh"));
    CHECK(refreshQueue.GetMemory("refresh") == FakeDeviceMemory(22));
    CHECK(refreshQueue.GetResidentMB() == 3);

    // The targeted BeginUpload already removed low from the priority queue; its failure is terminal.
    CHECK(queue.GetState("low") == StreamState::Failed);
    CHECK(queue.GetAssetCount() == 1);

    CHECK(queue.Enqueue(StreamRequest{"old", "old.obj", 2.0f, 4, 1}));
    CHECK(queue.TryDequeue(next));
    CHECK(next.id == "old");
    CHECK(queue.CompleteUpload("old", FakeDeviceMemory(2), 4));
    queue.MarkAccessed("high", 20);
    queue.MarkAccessed("old", 10);
    CHECK(queue.GetResidentMB() == 7);

    CHECK(queue.Enqueue(StreamRequest{"over", "over.obj", 1.0f, 1, 1}));
    CHECK(queue.TryDequeue(next));
    CHECK(next.id == "over");
    // Total resident budget, not just per-allocation budget, is the contract.
    CHECK(!queue.CompleteUpload("over", FakeDeviceMemory(3), 2));
    CHECK(queue.GetState("over") == StreamState::Uploading);
    CHECK(queue.FailUpload("over"));
    CHECK(queue.GetResidentMB() == 7);

    queue.SetMemoryBudgetMB(4);
    CHECK(queue.EvictToBudget());
    CHECK(queue.GetResidentMB() <= 4);
    CHECK(queue.IsReady("high"));
    CHECK(!queue.IsReady("old"));
    CHECK(released.size() == 1);
    CHECK(released[0] == FakeDeviceMemory(2));

    CHECK(queue.Release("high"));
    CHECK(queue.GetResidentMB() == 0);
    CHECK(!queue.Release("high"));
    CHECK(released.size() == 2);
    CHECK(released[1] == FakeDeviceMemory(1));

    // Existing allocations retain the releaser that owned them even if the
    // queue callback is replaced later (e.g. after a Vulkan device recreation).
    AssetStreamingQueue ownershipQueue(8, 8);
    std::vector<VkDeviceMemory> ownerA;
    std::vector<VkDeviceMemory> ownerB;
    ownershipQueue.SetGpuMemoryReleaseCallback([&ownerA](VkDeviceMemory memory) { ownerA.push_back(memory); });
    CHECK(ownershipQueue.Enqueue(StreamRequest{"owned", "owned.obj", 1.0f, 2, 1}));
    CHECK(ownershipQueue.TryDequeue(next));
    CHECK(next.id == "owned");
    CHECK(ownershipQueue.CompleteUpload("owned", FakeDeviceMemory(11), 2));
    ownershipQueue.SetGpuMemoryReleaseCallback([&ownerB](VkDeviceMemory memory) { ownerB.push_back(memory); });
    CHECK(ownershipQueue.Release("owned"));
    CHECK(ownerA.size() == 1 && ownerA[0] == FakeDeviceMemory(11));
    CHECK(ownerB.empty());

    // A throwing releaser must not make an allocation disappear or corrupt
    // resident accounting. A shared state lets the test recover and retry.
    AssetStreamingQueue retryQueue(8, 8);
    auto throwOnce = std::make_shared<bool>(true);
    std::vector<VkDeviceMemory> retryReleased;
    retryQueue.SetGpuMemoryReleaseCallback([throwOnce, &retryReleased](VkDeviceMemory memory) {
        if (*throwOnce) { *throwOnce = false; throw 1; }
        retryReleased.push_back(memory);
    });
    CHECK(retryQueue.Enqueue(StreamRequest{"retry", "retry.obj", 1.0f, 2, 1}));
    CHECK(retryQueue.TryDequeue(next));
    CHECK(retryQueue.CompleteUpload("retry", FakeDeviceMemory(12), 2));
    CHECK(!retryQueue.Release("retry"));
    CHECK(retryQueue.IsReady("retry"));
    CHECK(retryQueue.GetMemory("retry") == FakeDeviceMemory(12));
    CHECK(retryQueue.GetResidentMB() == 2);
    CHECK(retryQueue.Release("retry"));
    CHECK(retryReleased.size() == 1 && retryReleased[0] == FakeDeviceMemory(12));
    CHECK(retryQueue.GetResidentMB() == 0);

    std::cout << "ASSET_STREAMING_QUEUE_SMOKE_OK\n";
    return 0;
}

