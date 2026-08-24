#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NeoEngine {

struct NetworkPacket {
    std::string type;
    std::string data;
    int senderId;
    float timestamp;
};

class NetworkComponentCore {
public:
    NetworkComponentCore() = default;

    void SetPlayerId(int id) { m_PlayerId = id; }
    int GetPlayerId() const { return m_PlayerId; }

    void SendPacket(const std::string& type, const std::string& data) {
        m_Outgoing.push_back({type, data, m_PlayerId, 0});
    }

    std::vector<NetworkPacket> GetIncoming() {
        std::vector<NetworkPacket> packets = m_Incoming;
        m_Incoming.clear();
        return packets;
    }

    void SimulateLatency(std::vector<NetworkPacket>& packets, float latency) {
        for (auto& p : packets) p.timestamp += latency;
    }

private:
    int m_PlayerId = -1;
    std::vector<NetworkPacket> m_Outgoing;
    std::vector<NetworkPacket> m_Incoming;
};

} // namespace NeoEngine
