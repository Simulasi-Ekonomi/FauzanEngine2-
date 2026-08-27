#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

enum class ServerState : uint8_t { Offline, Available, Full, InGame };

struct NetworkServerDescriptor {
    uint64_t serverId{};
    uint64_t sessionId{};
    uint16_t protocolVersion{};
    uint16_t port{};
    uint32_t capacity{};
    uint32_t playerCount{};
    uint32_t regionCode{};
    ServerState state{ServerState::Offline};
};

class NetworkServerDiscovery {
public:
    static constexpr uint16_t ProtocolVersion = 1;

    static bool compatible(const NetworkServerDescriptor& server, uint16_t clientProtocol, uint32_t requestedRegion = 0) {
        if (!server.serverId || !server.sessionId || server.protocolVersion != clientProtocol) return false;
        if (requestedRegion && server.regionCode != requestedRegion) return false;
        if (server.playerCount >= server.capacity) return false;
        return server.state == ServerState::Available;
    }
};

} // namespace NeoEngine::Networking
