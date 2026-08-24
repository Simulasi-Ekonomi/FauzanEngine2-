#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct Artifact {
    std::string id, name, type;
    int level=1, maxLevel=20;
    float mainStat=0;
    std::unordered_map<std::string, float> subStats;
    std::string setId;
};

struct ArtifactSet {
    std::string id, name;
    std::vector<int> thresholds; // berapa piece untuk efek
    std::vector<std::string> effects;
};

class ArtifactSystem {
private:
    std::vector<Artifact> m_Artifacts;
    std::vector<ArtifactSet> m_Sets;

public:
    Artifact* CreateArtifact(const std::string& name, const std::string& type, const std::string& setId) {
        m_Artifacts.push_back({"art_"+std::to_string(m_Artifacts.size()), name, type, 1, 20}); return &m_Artifacts.back();
    }
    bool UpgradeArtifact(const std::string& id) {
        for (auto& a : m_Artifacts) if (a.id == id && a.level < a.maxLevel) { a.level++; a.mainStat *= 1.1f; return true; }
        return false;
    }
    void CreateSet(const std::string& name, const std::vector<int>& thresholds, const std::vector<std::string>& effects) {
        m_Sets.push_back({"set_"+std::to_string(m_Sets.size()), name, thresholds, effects});
    }
};

} // namespace NeoEngine
