#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct Blueprint {
    std::string id, name;
    std::string buildingType;
    int gridWidth = 1, gridHeight = 1;
    std::vector<std::string> requiredMaterials;
    std::vector<int> requiredCounts;
    int buildTime = 30;
    bool unlocked = false;
};

struct BlueprintPlacement {
    std::string blueprintId;
    float posX, posY, posZ;
    float progress = 0;
    bool completed = false;
};

class BlueprintSystem {
private:
    std::vector<Blueprint> m_Blueprints;
    std::unordered_map<std::string, std::vector<BlueprintPlacement>> m_Placements;
    std::function<void(const std::string&, const BlueprintPlacement&)> m_OnBuildComplete;
    
public:
    BlueprintSystem() {
        m_Blueprints = {
            {"house_small", "Small House", "house", 2, 2, {"wood", "stone"}, {20, 10}, 30, true},
            {"house_medium", "Medium House", "house", 3, 3, {"wood", "stone", "iron"}, {40, 20, 5}, 60, false},
            {"workshop", "Workshop", "workshop", 2, 2, {"wood", "stone", "iron"}, {30, 15, 10}, 45, true},
            {"warehouse", "Warehouse", "storage", 3, 3, {"wood", "stone"}, {50, 30}, 90, true},
            {"castle", "Castle", "castle", 5, 5, {"wood", "stone", "iron", "gold"}, {100, 80, 50, 20}, 300, false},
            {"tower_defense", "Defense Tower", "tower", 1, 1, {"stone", "iron"}, {30, 15}, 15, true},
        };
    }
    
    void UnlockBlueprint(const std::string& playerId, const std::string& bpId) {
        for (auto& b : m_Blueprints) if (b.id == bpId) b.unlocked = true;
    }
    
    BlueprintPlacement* PlaceBlueprint(const std::string& playerId, const std::string& bpId, float x, float y, float z) {
        for (auto& b : m_Blueprints) {
            if (b.id == bpId) {
                m_Placements[playerId].push_back({bpId, x, y, z, 0, false});
                return &m_Placements[playerId].back();
            }
        }
        return nullptr;
    }
    
    void UpdateBuilding(const std::string& playerId, int index, float progress) {
        auto it = m_Placements.find(playerId);
        if (it == m_Placements.end() || index >= it->second.size()) return;
        auto& p = it->second[index];
        p.progress += progress;
        if (p.progress >= 100.0f && !p.completed) {
            p.completed = true;
            p.progress = 100.0f;
            if (m_OnBuildComplete) m_OnBuildComplete(playerId, p);
        }
    }
    
    const std::vector<Blueprint>& GetBlueprints() const { return m_Blueprints; }
    const std::vector<BlueprintPlacement>* GetPlacements(const std::string& playerId) const {
        auto it = m_Placements.find(playerId);
        return it != m_Placements.end() ? &it->second : nullptr;
    }
    void SetOnBuildComplete(std::function<void(const std::string&, const BlueprintPlacement&)> cb) { m_OnBuildComplete = cb; }
};

} // namespace NeoEngine
