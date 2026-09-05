#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

namespace NeoEngine {

struct Asset {
    std::string name;
    std::string path;
    std::string type;
    void* data = nullptr;
    size_t size = 0;
    bool loaded = false;
};

class AssetManagerCore {
public:
    AssetManagerCore() = default;

    Asset* LoadAsset(const std::string& path) {
        if (m_Assets.find(path) != m_Assets.end()) return &m_Assets[path];
        Asset asset{path, path, "unknown", nullptr, 0, true};
        m_Assets[path] = asset;
        return &m_Assets[path];
    }

    void UnloadAsset(const std::string& path) {
        m_Assets.erase(path);
    }

    Asset* GetAsset(const std::string& path) {
        auto it = m_Assets.find(path);
        return it != m_Assets.end() ? &it->second : nullptr;
    }

    std::vector<std::string> GetAssetsByType(const std::string& type) {
        std::vector<std::string> result;
        for (auto& [path, asset] : m_Assets) {
            if (asset.type == type) result.push_back(path);
        }
        return result;
    }

    size_t GetAssetCount() const { return m_Assets.size(); }

private:
    std::unordered_map<std::string, Asset> m_Assets;
};

} // namespace NeoEngine

int main() {
    NeoEngine::AssetManagerCore manager;

    // Test 1: Load asset
    auto* asset1 = manager.LoadAsset("asset1.bin");
    assert(asset1 != nullptr && "Failed to load asset");
    assert(asset1->loaded && "Asset not marked loaded");
    assert(manager.GetAssetCount() == 1 && "Asset count mismatch");

    // Test 2: Get asset
    auto* retrieved = manager.GetAsset("asset1.bin");
    assert(retrieved == asset1 && "Retrieved asset mismatch");

    // Test 3: Load multiple assets
    manager.LoadAsset("asset2.bin");
    manager.LoadAsset("asset3.bin");
    assert(manager.GetAssetCount() == 3 && "Multiple asset load failed");

    // Test 4: Unload asset
    manager.UnloadAsset("asset2.bin");
    assert(manager.GetAssetCount() == 2 && "Asset unload failed");
    assert(manager.GetAsset("asset2.bin") == nullptr && "Unloaded asset still accessible");

    return 0; // All tests passed
}
