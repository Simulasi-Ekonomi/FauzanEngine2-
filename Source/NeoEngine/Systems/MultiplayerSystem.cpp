#include "MultiplayerSystem.h"
#include <android/log.h>

#define LOG_TAG_MP "Multiplayer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_MP, __VA_ARGS__)

namespace NeoEngine {

bool MultiplayerSystem::HostServer(int port) { m_IsHosting = m_Connected = true; LOGI("Host on port %d", port); return true; }
bool MultiplayerSystem::ConnectToServer(const std::string& ip, int port, const std::string& name) {
    m_Connected = true; m_LocalPlayerId = name; LOGI("Connected %s:%d", ip.c_str(), port); return true;
}
void MultiplayerSystem::Disconnect() { m_Connected = m_IsHosting = false; m_Players.clear(); }
void MultiplayerSystem::Update(float dt) {}
void MultiplayerSystem::SendMessage(const std::string& id, const std::string& msg) { if (OnMessageReceived) OnMessageReceived(id, msg); }
void MultiplayerSystem::BroadcastMessage(const std::string& msg) { for (auto& p : m_Players) if (p.connected) SendMessage(p.playerId, msg); }

} // namespace NeoEngine
