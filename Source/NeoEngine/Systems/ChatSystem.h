#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
    std::vector<std::pair<std::string, std::string>> m_Blocked;
    int m_MaxHistory = 500;
    int m_SpamCount = 0;
    float m_SpamTimer = 0;
    std::function<void(const ChatMessage&)> m_OnMessage;
    std::unordered_map<std::string, float> m_LastMessageTime;
    std::unordered_map<std::string, int> m_SpamCounts;
    static constexpr int SPAM_LIMIT = 5;
    static constexpr float SPAM_WINDOW = 3.0f;

    static float WallClockSeconds() noexcept {
        const auto now = std::chrono::system_clock::now();
        return std::chrono::duration<float>(now.time_since_epoch()).count();
    }

    static float MonotonicSeconds() noexcept {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float>(now.time_since_epoch()).count();
    }

public:
    bool SendMessage(const std::string& fromId, const std::string& fromName,
                     const std::string& text, ChatChannel channel = ChatChannel::Global,
                     const std::string& toId = "") {
        if (IsMuted(fromId)) return false;
        if (IsBlocked(fromId)) return false;

        const float currentTime = MonotonicSeconds();
        auto it = m_LastMessageTime.find(fromId);
        if (it != m_LastMessageTime.end()) {
            int& spamCount = m_SpamCounts[fromId];
            if (currentTime - it->second < 1.0f) {
                ++spamCount;
                if (spamCount >= SPAM_LIMIT) {
                    MutePlayer(fromId, 300.0f);
                    return false;
                }
            } else {
                spamCount = 0;
            }
        }
        m_LastMessageTime[fromId] = currentTime;

        ChatMessage msg{fromId, fromName, toId, text, channel, WallClockSeconds(), false};
        m_History.push_back(msg);
        if (m_History.size() > static_cast<std::size_t>(std::max(0, m_MaxHistory))) {
            m_History.erase(m_History.begin());
        }
        m_Pending.push_back(msg);
        if (m_OnMessage) m_OnMessage(msg);
        return true;
    }

    void SendSystemMessage(const std::string& text) {
        ChatMessage msg{"system", "System", "", text, ChatChannel::System,
                        WallClockSeconds(), true};
        m_History.push_back(msg);
        if (m_History.size() > static_cast<std::size_t>(std::max(0, m_MaxHistory))) {
            m_History.erase(m_History.begin());
        }
        if (m_OnMessage) m_OnMessage(msg);
    }

    void MutePlayer(const std::string& playerId, float durationSeconds) {
        if (!std::isfinite(durationSeconds) || durationSeconds <= 0.0f) return;
        m_Muted.push_back({playerId, durationSeconds, MonotonicSeconds()});
    }

    bool IsMuted(const std::string& playerId) const {
        const float now = MonotonicSeconds();
        for (const auto& m : m_Muted) {
            if (m.playerId == playerId && (now - m.startTime) < m.duration) return true;
        }
        return false;
    }

    void BlockPlayer(const std::string& fromId, const std::string& blockedId) {
        m_Blocked.emplace_back(blockedId, fromId);
    }

    bool IsBlocked(const std::string& playerId, const std::string& byId = "") const {
        for (const auto& blocked : m_Blocked) {
            if (blocked.first == playerId && (byId.empty() || blocked.second == byId)) return true;
        }
        return false;
    }

    std::vector<ChatMessage> GetChannelMessages(ChatChannel channel, int count = 20) const {
        std::vector<ChatMessage> result;
        if (count <= 0) return result;
        const std::size_t requested = static_cast<std::size_t>(count);
        const std::size_t start = m_History.size() > requested ? m_History.size() - requested : 0;
        for (std::size_t i = start; i < m_History.size(); ++i) {
            if (m_History[i].channel == channel || channel == ChatChannel::Global) {
                result.push_back(m_History[i]);
            }
        }
        return result;
    }

    std::vector<ChatMessage> GetWhispers(const std::string& playerId) const {
        std::vector<ChatMessage> result;
        for (const auto& m : m_History) {
            if (m.channel == ChatChannel::Whisper && (m.fromId == playerId || m.toId == playerId)) {
                result.push_back(m);
            }
        }
        return result;
    }

    void Clear() { m_History.clear(); m_Pending.clear(); }
    void SetOnMessage(std::function<void(const ChatMessage&)> cb) { m_OnMessage = std::move(cb); }
    const std::vector<ChatMessage>& GetHistory() const { return m_History; }
};

} // namespace NeoEngine
