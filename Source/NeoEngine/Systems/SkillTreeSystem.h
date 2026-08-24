#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {
struct SkillNode { std::string id, name; int maxLevel=5, currentLevel=0; int costPerLevel=1; std::vector<std::string> prerequisites; std::string unlocks; float valuePerLevel=1.0f; };
struct SkillTree { std::string name; std::vector<SkillNode> nodes; int skillPoints=0; };

class SkillTreeSystem {
private:
    std::unordered_map<std::string, SkillTree> m_Trees;
    std::function<void(const std::string&, const SkillNode&)> m_OnSkillLearned;
public:
    void CreateTree(const std::string& playerId, const std::string& name) { m_Trees[playerId] = {name, {}, 5}; }
    void AddNode(const std::string& treeId, const std::string& name, int maxLevel, int cost, const std::string& unlocks="", float value=1.0f) {
        auto it = m_Trees.find(treeId); if (it == m_Trees.end()) return;
        it->second.nodes.push_back({name, name, maxLevel, 0, cost, {}, unlocks, value});
    }
    bool LearnSkill(const std::string& playerId, const std::string& nodeName) {
        auto it = m_Trees.find(playerId); if (it == m_Trees.end()) return false;
        auto& tree = it->second; if (tree.skillPoints <= 0) return false;
        for (auto& node : tree.nodes) {
            if (node.name == nodeName && node.currentLevel < node.maxLevel) {
                node.currentLevel++; tree.skillPoints--;
                if (m_OnSkillLearned) m_OnSkillLearned(playerId, node);
                return true;
            }
        }
        return false;
    }
    int GetSkillPoints(const std::string& id) const { auto it = m_Trees.find(id); return it != m_Trees.end() ? it->second.skillPoints : 0; }
    void AddSkillPoints(const std::string& id, int pts) { auto it = m_Trees.find(id); if (it != m_Trees.end()) it->second.skillPoints += pts; }
    const SkillTree* GetTree(const std::string& id) const { auto it = m_Trees.find(id); return it != m_Trees.end() ? &it->second : nullptr; }
    void SetOnSkillLearned(std::function<void(const std::string&, const SkillNode&)> cb) { m_OnSkillLearned = cb; }
};
}
