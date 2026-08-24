#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NeoEngine {

struct PlayerSession {
    std::string playerId, playerName, ip;
    int port;
    bool connected = false;
    float lastPing = 0;
};

class MultiplayerSystem {
public:
    bool HostServer(int port);
    bool ConnectToServer(const std::string& ip, int port, const std::string& playerName);
    void Disconnect();
    void Update(float dt);
    void SendMessage(const std::string& playerId, const std::string& message);
    void BroadcastMessage(const std::string& message);
    const std::vector<PlayerSession>& GetPlayers() const { return m_Players; }
    bool IsHosting() const { return m_IsHosting; }

    std::function<void(const std::string&, const std::string&)> OnMessageReceived;
    std::function<void(const std::string&)> OnPlayerConnected;
    std::function<void(const std::string&)> OnPlayerDisconnected;

private:
    bool m_IsHosting = false;
    bool m_Connected = false;
    std::vector<PlayerSession> m_Players;
    std::string m_LocalPlayerId;
};

} // namespace NeoEngine
