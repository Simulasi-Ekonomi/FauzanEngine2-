#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <algorithm>

namespace NeoEngine {

enum class ChatChannel { Global, Guild, Party, Whisper, System, Trade, Local };

struct ChatMessage {
    std::string fromId, fromName, toId, text;
    ChatChannel channel = ChatChannel::Global;
    float timestamp = 0;
    bool isRead = false;
};

struct ChatMute {
    std::string playerId;
    float duration = 0;
    float startTime = 0;
};

class ChatSystem {
private:
    std::vector<ChatMessage> m_History;
    std::vector<ChatMessage> m_Pending;
    std::vector<ChatMute> m_Muted;
    std::vector<std::string> m_Blocked;
    int m_MaxHistory = 500;
    int m_SpamCount = 0;
    float m_SpamTimer = 0;
    std::function<void(const ChatMessage&)> m_OnMessage;
    std::unordered_map<std::string, float> m_LastMessageTime;
    static constexpr int SPAM_LIMIT = 5;
    static constexpr float SPAM_WINDOW = 3.0f;
    
public:
    bool SendMessage(const std::string& fromId, const std::string& fromName, 
                     const std::string& text, ChatChannel channel = ChatChannel::Global,
                     const std::string& toId = "") {
        if (IsMuted(fromId)) return false;
        if (IsBlocked(fromId)) return false;
        
        // Cek spam
        auto now = std::chrono::system_clock::now();
        float currentTime = std::chrono::duration<float>(now.time_since_epoch()).count();
        auto it = m_LastMessageTime.find(fromId);
        if (it != m_LastMessageTime.end()) {
            if (currentTime - it->second < 1.0f) {
                m_SpamCount++;
                if (m_SpamCount >= SPAM_LIMIT) {
                    MutePlayer(fromId, 300.0f);
                    return false;
                }
            } else {
                m_SpamCount = 0;
            }
        }
        m_LastMessageTime[fromId] = currentTime;
        
        ChatMessage msg{fromId, fromName, toId, text, channel, currentTime, false};
        m_History.push_back(msg);
        if (m_History.size() > m_MaxHistory) m_History.erase(m_History.begin());
        m_Pending.push_back(msg);
        if (m_OnMessage) m_OnMessage(msg);
        return true;
    }
    
    void SendSystemMessage(const std::string& text) {
        ChatMessage msg{"system", "System", "", text, ChatChannel::System, 
                       (float)std::chrono::system_clock::now().time_since_epoch().count(), true};
        m_History.push_back(msg);
        if (m_OnMessage) m_OnMessage(msg);
    }
    
    void MutePlayer(const std::string& playerId, float durationSeconds) {
        float now = std::chrono::system_clock::now().time_since_epoch().count();
        m_Muted.push_back({playerId, durationSeconds, now});
    }
    
    bool IsMuted(const std::string& playerId) const {
        float now = std::chrono::system_clock::now().time_since_epoch().count();
        for (auto& m : m_Muted) {
            if (m.playerId == playerId && (now - m.startTime) < m.duration) return true;
        }
        return false;
    }
    
    void BlockPlayer(const std::string& fromId, const std::string& blockedId) {
        m_Blocked.push_back(blockedId + ":" + fromId);
    }
    
    bool IsBlocked(const std::string& playerId, const std::string& byId = "") const {
        for (auto& b : m_Blocked) {
            if (b.find(playerId) != std::string::npos) return true;
        }
        return false;
    }
    
    std::vector<ChatMessage> GetChannelMessages(ChatChannel channel, int count = 20) const {
        std::vector<ChatMessage> result;
        int start = std::max(0, (int)m_History.size() - count);
        for (size_t i = start; i < m_History.size(); i++) {
            if (m_History[i].channel == channel || channel == ChatChannel::Global) {
                result.push_back(m_History[i]);
            }
        }
        return result;
    }
    
    std::vector<ChatMessage> GetWhispers(const std::string& playerId) const {
        std::vector<ChatMessage> result;
        for (auto& m : m_History) {
            if (m.channel == ChatChannel::Whisper && (m.fromId == playerId || m.toId == playerId)) {
                result.push_back(m);
            }
        }
        return result;
    }
    
    void Clear() { m_History.clear(); m_Pending.clear(); }
    void SetOnMessage(std::function<void(const ChatMessage&)> cb) { m_OnMessage = cb; }
    const std::vector<ChatMessage>& GetHistory() const { return m_History; }
};

} // namespace NeoEngine
