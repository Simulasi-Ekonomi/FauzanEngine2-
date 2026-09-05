#include <cassert>
#include <vector>
#include <string>

namespace NeoEngine {

using AssetID = std::string;

enum class StreamState : uint8_t {
    Pending, Uploading, Ready, Failed
};

struct StreamRequest {
    AssetID id;
    std::string filepath;
    float priority = 5.0f;
    uint32_t estimatedSizeMB = 1;
};

class AssetStreamingQueue {
public:
    [[nodiscard]] bool Enqueue(const StreamRequest& req) noexcept {
        if (req.id.empty()) return false;
        queuedAssets_.push_back(req.id);
        return true;
    }

    [[nodiscard]] bool IsReady(AssetID id) const noexcept {
        return std::find(readyAssets_.begin(), readyAssets_.end(), id) != readyAssets_.end();
    }

    void MarkReady(AssetID id) noexcept {
        if (!IsReady(id)) {
            readyAssets_.push_back(id);
        }
    }

    [[nodiscard]] uint32_t GetQueuedCount() const noexcept { return queuedAssets_.size(); }
    [[nodiscard]] uint32_t GetReadyCount() const noexcept { return readyAssets_.size(); }

private:
    std::vector<AssetID> queuedAssets_;
    std::vector<AssetID> readyAssets_;
};

} // namespace NeoEngine

int main() {
    NeoEngine::AssetStreamingQueue queue;

    // Test 1: Enqueue asset
    NeoEngine::StreamRequest req1{"mesh_01", "assets/mesh_01.bin", 10.0f};
    assert(queue.Enqueue(req1));
    assert(queue.GetQueuedCount() == 1);

    // Test 2: Multiple enqueues
    queue.Enqueue({"mesh_02", "assets/mesh_02.bin", 8.0f});
    queue.Enqueue({"mesh_03", "assets/mesh_03.bin", 5.0f});
    assert(queue.GetQueuedCount() == 3);

    // Test 3: Ready state transitions
    queue.MarkReady("mesh_01");
    assert(queue.IsReady("mesh_01"));
    assert(!queue.IsReady("mesh_02"));
    assert(queue.GetReadyCount() == 1);

    // Test 4: Empty queue handling
    NeoEngine::StreamRequest emptyReq{};
    assert(!queue.Enqueue(emptyReq));

    return 0;  // All tests passed
}
