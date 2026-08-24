#include "Systems/AuthorityLoopbackServer.h"
#include "Systems/AuthorityWireProtocol.h"
#include "Systems/TrustSafetySystem.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <vector>

namespace {
bool SendAll(int fd, const uint8_t* bytes, size_t count) { while (count > 0) { const ssize_t sent = send(fd, bytes, count, MSG_NOSIGNAL); if (sent <= 0) return false; bytes += sent; count -= static_cast<size_t>(sent); } return true; }
bool ReceiveAll(int fd, uint8_t* bytes, size_t count) { while (count > 0) { const ssize_t received = recv(fd, bytes, count, 0); if (received <= 0) return false; bytes += received; count -= static_cast<size_t>(received); } return true; }
}

int main() {
    using namespace NeoEngine;
    TrustSafetySystem trust;
    AuthoritativeCommandGate gate;
    if (!gate.Initialize(trust) || !gate.BindSession("player-loop", "session-loop")) return 1;
    std::atomic<uint32_t> handled{0};
    AuthorityLoopbackServer server;
    if (!server.Start(gate, 10, [&handled](const AuthorityCommand&, uint64_t) { ++handled; return true; }) || server.Port() == 0) return 1;
    const int client = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(server.Port());
    if (client < 0 || connect(client, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) return 1;
    AuthorityCommand command{"player-loop", "session-loop", "command-loop", "farm.harvest", 1, 10, {7}};
    std::vector<uint8_t> frame;
    AuthorityWireError wireError{};
    if (!AuthorityWireProtocol::EncodeCommand(command, frame, wireError)) return 1;
    const uint32_t length = static_cast<uint32_t>(frame.size());
    const std::array<uint8_t, 4> header{static_cast<uint8_t>(length >> 24U), static_cast<uint8_t>(length >> 16U), static_cast<uint8_t>(length >> 8U), static_cast<uint8_t>(length)};
    std::array<uint8_t, 4> responseHeader{};
    if (!SendAll(client, header.data(), header.size()) || !SendAll(client, frame.data(), frame.size()) || !ReceiveAll(client, responseHeader.data(), responseHeader.size())) return 1;
    const uint32_t responseSize = (static_cast<uint32_t>(responseHeader[0]) << 24U) | (static_cast<uint32_t>(responseHeader[1]) << 16U) | (static_cast<uint32_t>(responseHeader[2]) << 8U) | responseHeader[3];
    if (responseSize == 0 || responseSize > AuthorityWireProtocol::kMaxSnapshotBytes + 32U) return 1;
    std::vector<uint8_t> response(responseSize);
    if (!ReceiveAll(client, response.data(), response.size())) return 1;
    close(client);
    server.Stop();
    AuthorityWireSnapshot snapshot{};
    if (!AuthorityWireProtocol::DecodeSnapshot(response, snapshot, wireError) || snapshot.revision != 1 || snapshot.state != std::vector<uint8_t>{1} || handled.load() != 1 || gate.AuthoritativeRevision() != 1) return 1;
    AuthorityLoopbackServer idleServer;
    if (!idleServer.Start(gate, 11, [](const AuthorityCommand&, uint64_t) { return true; }) || idleServer.Port() == 0) return 1;
    const int idleClient = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_port = htons(idleServer.Port());
    if (idleClient < 0 || connect(idleClient, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) return 1;
    const auto start = std::chrono::steady_clock::now();
    idleServer.Stop();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    close(idleClient);
    if (elapsed > std::chrono::seconds(1)) return 1;
    std::printf("AUTHORITY_LOOPBACK_TRANSPORT_SMOKE_OK port=loopback revision=%llu handled=%u idleStop=1\n", static_cast<unsigned long long>(snapshot.revision), handled.load());
    return 0;
}
