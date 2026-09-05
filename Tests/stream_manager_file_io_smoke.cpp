#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

// Mock StreamManager for standalone test
namespace NeoEngine {

struct StreamRequest {
    std::string assetPath;
    int priority = 5;
    std::function<void(const std::vector<uint8_t>&)> onLoaded;
};

class StreamManager {
public:
    explicit StreamManager(int maxWorkers = 2) : m_MaxWorkers(maxWorkers) {}
    ~StreamManager() { Stop(); }

    void Start();
    void Stop();
    void RequestLoad(const std::string& path, int priority, 
                     std::function<void(const std::vector<uint8_t>&)> callback);
    
    const std::vector<uint8_t>* GetAsset(const std::string& path) const;
    bool IsLoaded(const std::string& path) const;
    void UnloadAsset(const std::string& path);

private:
    void WorkerLoop();
    std::vector<uint8_t> LoadAssetFile(const std::string& path) const;

    std::queue<StreamRequest> m_Queue;
    mutable std::unordered_map<std::string, struct StreamedAsset> m_LoadedAssets;
    std::vector<std::thread> m_Workers;
    mutable std::mutex m_Mutex;
    std::atomic<bool> m_Running{false};
    int m_MaxWorkers;
};

} // namespace NeoEngine

int main() {
    // Create test asset file
    const char* testFile = "/tmp/test_asset.bin";
    std::vector<uint8_t> testData = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    {
        std::ofstream out(testFile, std::ios::binary);
        out.write(reinterpret_cast<const char*>(testData.data()), testData.size());
    }

    NeoEngine::StreamManager manager(2);
    manager.Start();

    // Test 1: File I/O correctness
    bool callbackCalled = false;
    std::vector<uint8_t> loadedData;
    
    manager.RequestLoad(testFile, 5, [&](const std::vector<uint8_t>& data) {
        callbackCalled = true;
        loadedData = data;
    });

    // Wait for load
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    assert(callbackCalled && "Callback not called");
    assert(loadedData == testData && "Data mismatch");
    assert(manager.IsLoaded(testFile) && "Asset not marked loaded");

    // Test 2: GetAsset correctness
    const auto* asset = manager.GetAsset(testFile);
    assert(asset != nullptr && "Asset pointer null");
    assert(*asset == testData && "Asset data mismatch");

    // Test 3: Unload
    manager.UnloadAsset(testFile);
    assert(!manager.IsLoaded(testFile) && "Asset not unloaded");
    assert(manager.GetAsset(testFile) == nullptr && "Unloaded asset still accessible");

    manager.Stop();
    std::remove(testFile);

    return 0; // All tests passed
}
