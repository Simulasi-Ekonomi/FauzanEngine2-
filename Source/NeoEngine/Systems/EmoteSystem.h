#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct Emote {
    std::string id, name, animation;
    bool unlocked = false;
    int cost = 0;
    std::string rarity = "common";
    float duration = 3.0f;
};

struct EmoteAction {
    std::string playerId;
    std::string emoteId;
    float posX, posY, posZ;
    float timestamp;
};

class EmoteSystem {
private:
    std::vector<Emote> m_Emotes;
    std::unordered_map<std::string, std::vector<std::string>> m_OwnedEmotes;
    std::vector<EmoteAction> m_RecentActions;
    std::function<void(const EmoteAction&)> m_OnEmote;
    
public:
    EmoteSystem() {
        m_Emotes = {
            {"wave", "Wave", "wave_anim", true, 0, "common", 2.0f},
            {"clap", "Clap", "clap_anim", true, 0, "common", 2.0f},
            {"dance", "Dance", "dance_anim", true, 0, "common", 4.0f},
            {"laugh", "Laugh", "laugh_anim", true, 0, "common", 3.0f},
            {"cry", "Cry", "cry_anim", true, 0, "common", 3.0f},
            {"salute", "Salute", "salute_anim", false, 100, "rare", 2.0f},
            {"bow", "Bow", "bow_anim", false, 50, "uncommon", 2.5f},
            {"flex", "Flex", "flex_anim", false, 200, "rare", 3.0f},
            {"heart", "Heart", "heart_anim", false, 150, "rare", 3.0f},
            {"taunt", "Taunt", "taunt_anim", false, 300, "epic", 3.5f},
            {"victory", "Victory", "victory_anim", false, 500, "legendary", 5.0f},
            {"disco", "Disco", "disco_anim", false, 800, "legendary", 6.0f},
        };
    }
    
    bool UnlockEmote(const std::string& playerId, const std::string& emoteId) {
        for (auto& e : m_Emotes) {
            if (e.id == emoteId) {
                e.unlocked = true;
                m_OwnedEmotes[playerId].push_back(emoteId);
                return true;
            }
        }
        return false;
    }
    
    bool UseEmote(const std::string& playerId, const std::string& emoteId, float x, float y, float z) {
        auto it = m_OwnedEmotes.find(playerId);
        if (it == m_OwnedEmotes.end()) return false;
        for (auto& id : it->second) {
            if (id == emoteId) {
                EmoteAction action{playerId, emoteId, x, y, z, 0};
                m_RecentActions.push_back(action);
                if (m_RecentActions.size() > 100) m_RecentActions.erase(m_RecentActions.begin());
                if (m_OnEmote) m_OnEmote(action);
                return true;
            }
        }
        return false;
    }
    
    const std::vector<Emote>& GetAvailableEmotes(const std::string& playerId) const {
        static std::vector<Emote> available;
        available.clear();
        auto it = m_OwnedEmotes.find(playerId);
        if (it != m_OwnedEmotes.end()) {
            for (auto& id : it->second) {
                for (auto& e : m_Emotes) {
                    if (e.id == id) available.push_back(e);
                }
            }
        }
        // Tambah yang free
        for (auto& e : m_Emotes) {
            if (e.unlocked && e.cost == 0) {
                bool found = false;
                for (auto& a : available) if (a.id == e.id) found = true;
                if (!found) available.push_back(e);
            }
        }
        return available;
    }
    
    const std::vector<Emote>& GetAllEmotes() const { return m_Emotes; }
    void SetOnEmote(std::function<void(const EmoteAction&)> cb) { m_OnEmote = cb; }
};

} // namespace NeoEngine
