#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

enum class QuestStatus { Available, Active, Completed, Failed };
enum class QuestType { Main, Side, Daily, Event, Achievement };

struct Quest {
    std::string id, title, description;
    QuestType type;
    QuestStatus status = QuestStatus::Available;
    std::vector<std::string> objectives;
    std::vector<int> objProgress;
    std::vector<int> objTargets;
    std::string rewardItem;
    int rewardXP = 0, rewardGold = 0;
};

class QuestSystem {
public:
    std::string AddQuest(const std::string& title, const std::string& desc, QuestType type) {
        std::string id = "q_" + std::to_string(m_NextId++);
        m_Quests[id] = {id, title, desc, type};
        return id;
    }

    void AddObjective(const std::string& id, const std::string& desc, int target) {
        auto it = m_Quests.find(id);
        if (it != m_Quests.end()) {
            it->second.objectives.push_back(desc);
            it->second.objProgress.push_back(0);
            it->second.objTargets.push_back(target);
        }
    }

    bool AcceptQuest(const std::string& id) {
        auto it = m_Quests.find(id);
        if (it != m_Quests.end() && it->second.status == QuestStatus::Available) {
            it->second.status = QuestStatus::Active;
            return true;
        }
        return false;
    }

    bool UpdateProgress(const std::string& id, int objIndex, int amount) {
        auto it = m_Quests.find(id);
        if (it == m_Quests.end() || it->second.status != QuestStatus::Active) return false;
        if (objIndex >= (int)it->second.objProgress.size()) return false;
        it->second.objProgress[objIndex] += amount;
        if (it->second.objProgress[objIndex] >= it->second.objTargets[objIndex]) {
            it->second.objProgress[objIndex] = it->second.objTargets[objIndex];
            bool done = true;
            for (size_t i = 0; i < it->second.objProgress.size(); ++i)
                if (it->second.objProgress[i] < it->second.objTargets[i]) { done = false; break; }
            if (done) {
                it->second.status = QuestStatus::Completed;
                if (m_OnComplete) m_OnComplete(it->second);
            }
        }
        return true;
    }

    void SetOnQuestComplete(std::function<void(const Quest&)> cb) { m_OnComplete = cb; }

private:
    std::unordered_map<std::string, Quest> m_Quests;
    std::function<void(const Quest&)> m_OnComplete;
    int m_NextId = 1;
};

} // namespace NeoEngine
