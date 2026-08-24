#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

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
