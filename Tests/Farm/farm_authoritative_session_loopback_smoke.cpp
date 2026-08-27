#include "Systems/AuthorityLoopbackServer.h"
#include "Systems/AuthorityWireProtocol.h"
#include "Systems/FarmAuthoritativeService.h"
#include "Systems/FarmAuthoritativeSessionHost.h"
#include "Systems/FarmAuthoritativeSessionLoopback.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

bool SendAll(int fd, const uint8_t* bytes, size_t count) {
    while (count > 0U) {
        const ssize_t sent = send(fd, bytes, count, MSG_NOSIGNAL);
        if (sent <= 0) return false;
        bytes += sent;
        count -= static_cast<size_t>(sent);
    }
    return true;
}

bool ReceiveAll(int fd, uint8_t* bytes, size_t count) {
    while (count > 0U) {
        const ssize_t received = recv(fd, bytes, count, 0);
        if (received <= 0) return false;
        bytes += received;
        count -= static_cast<size_t>(received);
    }
    return true;
}

int Connect(uint16_t port) {
    const int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) return -1;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    if (connect(client, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        close(client);
        return -1;
    }
    return client;
}

bool SendCommandAndReceiveSnapshot(int fd, const NeoEngine::AuthorityCommand& command, NeoEngine::AuthorityWireSnapshot& snapshot) {
    NeoEngine::AuthorityWireError error{};
    std::vector<uint8_t> frame;
    if (!NeoEngine::AuthorityWireProtocol::EncodeCommand(command, frame, error)) return false;
    const uint32_t length = static_cast<uint32_t>(frame.size());
    const std::array<uint8_t, 4> header{static_cast<uint8_t>(length >> 24U), static_cast<uint8_t>(length >> 16U),
                                        static_cast<uint8_t>(length >> 8U), static_cast<uint8_t>(length)};
    std::array<uint8_t, 4> responseHeader{};
    if (!SendAll(fd, header.data(), header.size()) || !SendAll(fd, frame.data(), frame.size()) ||
        !ReceiveAll(fd, responseHeader.data(), responseHeader.size())) return false;
    const uint32_t responseLength = (static_cast<uint32_t>(responseHeader[0]) << 24U) |
                                    (static_cast<uint32_t>(responseHeader[1]) << 16U) |
                                    (static_cast<uint32_t>(responseHeader[2]) << 8U) | responseHeader[3];
    if (responseLength == 0U || responseLength > NeoEngine::AuthorityWireProtocol::kMaxSnapshotBytes + 32U) return false;
    std::vector<uint8_t> response(responseLength);
    return ReceiveAll(fd, response.data(), response.size()) && NeoEngine::AuthorityWireProtocol::DecodeSnapshot(response, snapshot, error);
}

bool WaitForStop(const NeoEngine::AuthorityLoopbackServer& transport) {
    for (uint8_t attempt = 0U; attempt < 50U; ++attempt) {
        if (!transport.IsRunning()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return !transport.IsRunning();
}

} // namespace

int main() {
    using namespace NeoEngine;
    FarmSystem farm(20U, 20U, 100U);
    TrustSafetySystem trust;
    FarmWorldTool world;
    FarmWorldConfig config{};
    config.worldWidth = 20U;
    config.worldHeight = 20U;
    if (!world.Initialize(farm, trust, "session-loop-player", config)) return 1;
    FarmAuthoritativeService authority;
    if (!authority.Initialize(world, trust, "session-loop-player", "bootstrap-session")) return 1;
    FarmAuthoritativeSessionHost host;
    if (!host.Initialize(authority)) return 1;

    AuthorityLoopbackServer invalidTransport;
    FarmAuthoritativeSessionLoopback invalidAdapter;
    FarmAuthoritativeSessionHost uninitializedHost;
    if (invalidAdapter.Start(uninitializedHost, invalidTransport, {"session-loop-player", "server-session-01"}, 10U) ||
        invalidAdapter.LastError() != FarmAuthoritativeSessionLoopbackError::InvalidConfiguration || invalidTransport.IsRunning()) return 2;

    AuthorityLoopbackServer transport;
    FarmAuthoritativeSessionLoopback adapter;
    const FarmSessionPrincipal principal{"session-loop-player", "server-session-01"};
    if (!adapter.Start(host, transport, principal, 10U, 2U) || adapter.Port() == 0U || !adapter.IsRunning()) return 3;
    const int validClient = Connect(adapter.Port());
    if (validClient < 0) return 3;
    const AuthorityCommand command{"session-loop-player", "server-session-01", "loopback-command-001", "farm.till", 1U, 10U, {2U, 0U, 2U, 0U}};
    AuthorityWireSnapshot first{}, replay{};
    if (!SendCommandAndReceiveSnapshot(validClient, command, first) || !SendCommandAndReceiveSnapshot(validClient, command, replay)) return 4;
    close(validClient);
    int reconnectClient = -1;
    for (uint8_t attempt = 0U; attempt < 50U && reconnectClient < 0; ++attempt) {
        reconnectClient = Connect(adapter.Port());
        if (reconnectClient < 0) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if (reconnectClient < 0) return 4;
    AuthorityWireSnapshot reconnectReplay{};
    if (!SendCommandAndReceiveSnapshot(reconnectClient, command, reconnectReplay)) return 4;
    close(reconnectClient);
    adapter.Stop();
    if (first.revision != 1U || replay.revision != 1U || reconnectReplay.revision != 1U || first.state != replay.state || first.state != reconnectReplay.state ||
        farm.TileStateAt(2U, 2U) != FarmTileState::Tilled ||
        transport.LastError() != AuthorityTransportError::None || adapter.IsRunning() || adapter.Port() != 0U) return 4;

    AuthorityLoopbackServer subjectTransport;
    FarmAuthoritativeSessionLoopback subjectAdapter;
    if (!subjectAdapter.Start(host, subjectTransport, {"session-loop-player", "server-session-02"}, 10U)) return 5;
    const int subjectClient = Connect(subjectAdapter.Port());
    if (subjectClient < 0) return 5;
    AuthorityWireSnapshot rejected{};
    const AuthorityCommand subjectSpoof{"different-player", "server-session-02", "loopback-command-002", "farm.till", 2U, 10U, {3U, 0U, 2U, 0U}};
    if (SendCommandAndReceiveSnapshot(subjectClient, subjectSpoof, rejected)) return 5;
    close(subjectClient);
    if (!WaitForStop(subjectTransport) || subjectAdapter.LastError() != FarmAuthoritativeSessionLoopbackError::WirePrincipalRejected ||
        subjectTransport.LastError() != AuthorityTransportError::WireRejected || farm.TileStateAt(3U, 2U) == FarmTileState::Tilled || authority.Revision() != 1U) return 5;
    subjectAdapter.Stop();

    AuthorityLoopbackServer sessionTransport;
    FarmAuthoritativeSessionLoopback sessionAdapter;
    if (!sessionAdapter.Start(host, sessionTransport, {"session-loop-player", "server-session-03"}, 10U)) return 6;
    const int sessionClient = Connect(sessionAdapter.Port());
    if (sessionClient < 0) return 6;
    const AuthorityCommand sessionSpoof{"session-loop-player", "server-session-02", "loopback-command-003", "farm.till", 2U, 10U, {3U, 0U, 2U, 0U}};
    if (SendCommandAndReceiveSnapshot(sessionClient, sessionSpoof, rejected)) return 6;
    close(sessionClient);
    if (!WaitForStop(sessionTransport) || sessionAdapter.LastError() != FarmAuthoritativeSessionLoopbackError::WirePrincipalRejected ||
        sessionTransport.LastError() != AuthorityTransportError::WireRejected || farm.TileStateAt(3U, 2U) == FarmTileState::Tilled || authority.Revision() != 1U) return 6;
    sessionAdapter.Stop();

    std::printf("FARM_AUTHORITATIVE_SESSION_LOOPBACK_SMOKE_OK revision=%llu replay=1 reconnect=1 subject_rejected=1 session_rejected=1\n",
                static_cast<unsigned long long>(authority.Revision()));
    return 0;
}
