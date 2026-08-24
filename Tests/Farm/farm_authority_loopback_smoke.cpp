#include "Systems/AuthorityLoopbackServer.h"
#include "Systems/AuthorityWireProtocol.h"
#include "Systems/FarmAuthoritativeService.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <vector>

namespace {
bool SendAll(int fd, const uint8_t* bytes, size_t count) { while (count > 0) { const ssize_t sent = send(fd, bytes, count, MSG_NOSIGNAL); if (sent <= 0) return false; bytes += sent; count -= static_cast<size_t>(sent); } return true; }
bool ReceiveAll(int fd, uint8_t* bytes, size_t count) { while (count > 0) { const ssize_t received = recv(fd, bytes, count, 0); if (received <= 0) return false; bytes += received; count -= static_cast<size_t>(received); } return true; }
bool SendCommandAndReceiveSnapshot(int fd, const NeoEngine::AuthorityCommand& command, NeoEngine::AuthorityWireSnapshot& snapshot) {
    NeoEngine::AuthorityWireError error{};
    std::vector<uint8_t> frame;
    if (!NeoEngine::AuthorityWireProtocol::EncodeCommand(command, frame, error)) return false;
    const uint32_t length = static_cast<uint32_t>(frame.size());
    const std::array<uint8_t, 4> header{static_cast<uint8_t>(length >> 24U), static_cast<uint8_t>(length >> 16U), static_cast<uint8_t>(length >> 8U), static_cast<uint8_t>(length)};
    std::array<uint8_t, 4> responseHeader{};
    if (!SendAll(fd, header.data(), header.size()) || !SendAll(fd, frame.data(), frame.size()) || !ReceiveAll(fd, responseHeader.data(), responseHeader.size())) return false;
    const uint32_t responseLength = (static_cast<uint32_t>(responseHeader[0]) << 24U) | (static_cast<uint32_t>(responseHeader[1]) << 16U) | (static_cast<uint32_t>(responseHeader[2]) << 8U) | responseHeader[3];
    if (responseLength == 0 || responseLength > NeoEngine::AuthorityWireProtocol::kMaxSnapshotBytes + 32U) return false;
    std::vector<uint8_t> response(responseLength);
    return ReceiveAll(fd, response.data(), response.size()) && NeoEngine::AuthorityWireProtocol::DecodeSnapshot(response, snapshot, error);
}
}

int main() {
    using namespace NeoEngine;
    FarmSystem farm(20, 20, 100);
    TrustSafetySystem trust;
    FarmWorldTool world;
    FarmWorldConfig config{};
    config.worldWidth = 20;
    config.worldHeight = 20;
    if (!world.Initialize(farm, trust, "farm-loop-player", config)) return 1;
    FarmAuthoritativeService service;
    if (!service.Initialize(world, trust, "farm-loop-player", "farm-loop-session")) return 1;
    AuthorityLoopbackServer server;
    if (!server.Start(
            [&service](const AuthorityCommand& command) { return service.Submit(command, 10); },
            [&service](const AuthorityDecision&, AuthorityWireSnapshot& snapshot) { FarmAuthoritySnapshot farmSnapshot{}; if (!service.BuildSnapshot(farmSnapshot)) return false; snapshot = {farmSnapshot.revision, std::move(farmSnapshot.worldBytes)}; return true; }) ||
        server.Port() == 0) return 1;
    const int client = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(server.Port());
    if (client < 0 || connect(client, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) return 1;
    const AuthorityCommand first{"farm-loop-player", "farm-loop-session", "farm-loop-cmd-001", "farm.till", 1, 10, {2, 0, 2, 0}};
    const AuthorityCommand second{"farm-loop-player", "farm-loop-session", "farm-loop-cmd-002", "farm.till", 2, 10, {3, 0, 2, 0}};
    const AuthorityCommand third{"farm-loop-player", "farm-loop-session", "farm-loop-cmd-003", "farm.till", 3, 10, {4, 0, 2, 0}};
    AuthorityWireSnapshot firstSnapshot{}, secondSnapshot{}, thirdSnapshot{}, replaySnapshot{};
    if (!SendCommandAndReceiveSnapshot(client, first, firstSnapshot) || !SendCommandAndReceiveSnapshot(client, second, secondSnapshot) || !SendCommandAndReceiveSnapshot(client, third, thirdSnapshot) || !SendCommandAndReceiveSnapshot(client, first, replaySnapshot)) return 1;
    close(client);
    server.Stop();
    FarmSystem reconciledFarm(20, 20, 100);
    TrustSafetySystem reconciledTrust;
    FarmWorldTool reconciledWorld;
    if (firstSnapshot.revision != 1 || secondSnapshot.revision != 2 || thirdSnapshot.revision != 3 || replaySnapshot.revision != 3 || !reconciledWorld.Initialize(reconciledFarm, reconciledTrust, "farm-loop-player", config) || !reconciledWorld.Deserialize(firstSnapshot.state) || reconciledFarm.TileStateAt(2, 2) != FarmTileState::Tilled || reconciledFarm.TileStateAt(3, 2) == FarmTileState::Tilled || !reconciledWorld.Deserialize(secondSnapshot.state) || reconciledFarm.TileStateAt(3, 2) != FarmTileState::Tilled || !reconciledWorld.Deserialize(thirdSnapshot.state) || reconciledFarm.TileStateAt(4, 2) != FarmTileState::Tilled || !reconciledWorld.Deserialize(replaySnapshot.state) || reconciledWorld.DeterministicState() != world.DeterministicState() || server.LastError() != AuthorityTransportError::None) return 1;
    std::printf("FARM_AUTHORITY_LOOPBACK_SMOKE_OK revision=%llu bytes=%zu commands=3 replay=1 reconciliation=1\n", static_cast<unsigned long long>(replaySnapshot.revision), replaySnapshot.state.size());
    return 0;
}
