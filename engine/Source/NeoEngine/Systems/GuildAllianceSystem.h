#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct GuildAlliance {
    std::string allianceId, allianceName;
    std::vector<std::string> memberGuildIds;
    std::vector<std::string> enemyAllianceIds;
    int totalPower = 0;
    int level = 1;
};

class GuildAllianceSystem {
private:
    std::vector<GuildAlliance> m_Alliances;
    std::function<void(const GuildAlliance&, const std::string&)> m_OnGuildJoin;
    std::function<void(const GuildAlliance&, const GuildAlliance&)> m_OnWarDeclared;
    
public:
    GuildAlliance* CreateAlliance(const std::string& name) {
        m_Alliances.push_back({"al_" + std::to_string(m_Alliances.size()), name, {}, {}, 0, 1});
        return &m_Alliances.back();
    }
    
    bool JoinAlliance(const std::string& allianceId, const std::string& guildId) {
        for (auto& a : m_Alliances) {
            if (a.allianceId == allianceId) {
                a.memberGuildIds.push_back(guildId);
                if (m_OnGuildJoin) m_OnGuildJoin(a, guildId);
                return true;
            }
        }
        return false;
    }
    
    bool DeclareWar(const std::string& allianceA, const std::string& allianceB) {
        GuildAlliance *a = nullptr, *b = nullptr;
        for (auto& al : m_Alliances) {
            if (al.allianceId == allianceA) a = &al;
            if (al.allianceId == allianceB) b = &al;
        }
        if (!a || !b) return false;
        a->enemyAllianceIds.push_back(allianceB);
        b->enemyAllianceIds.push_back(allianceA);
        if (m_OnWarDeclared) m_OnWarDeclared(*a, *b);
        return true;
    }
    
    GuildAlliance* GetAlliance(const std::string& id) {
        for (auto& a : m_Alliances) if (a.allianceId == id) return &a;
        return nullptr;
    }
    
    void SetOnGuildJoin(std::function<void(const GuildAlliance&, const std::string&)> cb) { m_OnGuildJoin = cb; }
    void SetOnWarDeclared(std::function<void(const GuildAlliance&, const GuildAlliance&)> cb) { m_OnWarDeclared = cb; }
};

} // namespace NeoEngine
