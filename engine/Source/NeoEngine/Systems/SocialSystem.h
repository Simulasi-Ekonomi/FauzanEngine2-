#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct Friend { std::string id, name; int level; bool online; float lastOnline; };
struct ChatMessage { std::string from, to, text; float timestamp; bool isGuild; };
struct Gift { std::string from, item; int count; float timestamp; };

class SocialSystem {
private:
    std::vector<Friend> m_Friends;
    std::vector<ChatMessage> m_ChatHistory;
    std::vector<Gift> m_PendingGifts;
    std::vector<std::string> m_BlockedPlayers;
    int m_MaxFriends = 100;
    std::function<void(const ChatMessage&)> m_OnMessage;
    std::function<void(const Gift&)> m_OnGift;

public:
    bool AddFriend(const std::string& id, const std::string& name, int level) {
        if (m_Friends.size() >= m_MaxFriends) return false;
        if (IsBlocked(id)) return false;
        m_Friends.push_back({id, name, level, false, 0});
        return true;
    }

    void RemoveFriend(const std::string& id) {
        m_Friends.erase(std::remove_if(m_Friends.begin(), m_Friends.end(),
            [&id](const Friend& f) { return f.id == id; }), m_Friends.end());
    }

    void SendMessage(const std::string& from, const std::string& to, const std::string& text) {
        if (IsBlocked(to)) return;
        ChatMessage msg{from, to, text, 0, false};
        m_ChatHistory.push_back(msg);
        if (m_OnMessage) m_OnMessage(msg);
    }

    void SendGuildMessage(const std::string& from, const std::string& text) {
        ChatMessage msg{from, "guild", text, 0, true};
        m_ChatHistory.push_back(msg);
        if (m_OnMessage) m_OnMessage(msg);
    }

    bool SendGift(const std::string& from, const std::string& to, const std::string& item, int count) {
        if (IsBlocked(to)) return false;
        m_PendingGifts.push_back({from, item, count, 0});
        return true;
    }

    Gift ClaimGift(const std::string& playerId) {
        for (auto it = m_PendingGifts.begin(); it != m_PendingGifts.end(); ++it) {
            if (it->from != playerId) {
                Gift g = *it;
                m_PendingGifts.erase(it);
                if (m_OnGift) m_OnGift(g);
                return g;
            }
        }
        return {};
    }

    void BlockPlayer(const std::string& id) { m_BlockedPlayers.push_back(id); }
    bool IsBlocked(const std::string& id) const {
        return std::find(m_BlockedPlayers.begin(), m_BlockedPlayers.end(), id) != m_BlockedPlayers.end();
    }

    const std::vector<Friend>& GetFriends() const { return m_Friends; }
    int GetFriendCount() const { return m_Friends.size(); }
    int GetOnlineCount() const {
        int count = 0;
        for (auto& f : m_Friends) if (f.online) count++;
        return count;
    }

    void SetOnMessage(std::function<void(const ChatMessage&)> cb) { m_OnMessage = cb; }
    void SetOnGift(std::function<void(const Gift&)> cb) { m_OnGift = cb; }
};

} // namespace NeoEngine
