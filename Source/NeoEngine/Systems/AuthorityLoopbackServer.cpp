#include "AuthorityLoopbackServer.h"

#include "AuthorityWireProtocol.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <vector>

namespace NeoEngine {
namespace {

bool SendAll(int fd, const uint8_t* bytes, size_t count) {
    while (count > 0) {
        const ssize_t sent = send(fd, bytes, count, MSG_NOSIGNAL);
        if (sent <= 0) return false;
        bytes += sent;
        count -= static_cast<size_t>(sent);
    }
    return true;
}

bool ReceiveAll(int fd, uint8_t* bytes, size_t count) {
    while (count > 0) {
        const ssize_t received = recv(fd, bytes, count, 0);
        if (received <= 0) return false;
        bytes += received;
        count -= static_cast<size_t>(received);
    }
    return true;
}

} // namespace

AuthorityLoopbackServer::~AuthorityLoopbackServer() {
    Stop();
}

bool AuthorityLoopbackServer::Start(AuthoritativeCommandGate& gate, uint64_t serverTick, Handler handler) {
    if (!gate.IsReady() || serverTick == 0 || !handler) {
        lastError_.store(AuthorityTransportError::InvalidConfiguration);
        return false;
    }
    return Start(
        [&gate, serverTick, handler = std::move(handler)](const AuthorityCommand& command) { return gate.Submit(command, serverTick, handler); },
        [&gate](const AuthorityDecision& decision, AuthorityWireSnapshot& snapshot) {
            snapshot = {decision.authoritativeRevision == 0 ? gate.AuthoritativeRevision() : decision.authoritativeRevision, {static_cast<uint8_t>(decision.Accepted() ? 1U : 0U)}};
            return snapshot.revision != 0;
        });
}

bool AuthorityLoopbackServer::Start(Dispatcher dispatcher, SnapshotBuilder snapshotBuilder) {
    Stop();
    if (!dispatcher || !snapshotBuilder) {
        lastError_.store(AuthorityTransportError::InvalidConfiguration);
        return false;
    }
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { lastError_.store(AuthorityTransportError::SocketOpenFailed); return false; }
    const int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    if (bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) { close(fd); lastError_.store(AuthorityTransportError::BindFailed); return false; }
    if (listen(fd, 1) != 0) { close(fd); lastError_.store(AuthorityTransportError::ListenFailed); return false; }
    socklen_t addressSize = sizeof(address);
    if (getsockname(fd, reinterpret_cast<sockaddr*>(&address), &addressSize) != 0 || ntohs(address.sin_port) == 0) { close(fd); lastError_.store(AuthorityTransportError::BindFailed); return false; }
    dispatcher_ = std::move(dispatcher);
    snapshotBuilder_ = std::move(snapshotBuilder);
    listenFd_ = fd;
    port_ = ntohs(address.sin_port);
    running_.store(true);
    lastError_.store(AuthorityTransportError::None);
    worker_ = std::thread(&AuthorityLoopbackServer::Run, this);
    return true;
}

void AuthorityLoopbackServer::Stop() {
    const int fd = listenFd_.exchange(-1);
    const int client = clientFd_.exchange(-1);
    running_.store(false);
    if (client >= 0) { shutdown(client, SHUT_RDWR); close(client); }
    if (fd >= 0) { shutdown(fd, SHUT_RDWR); close(fd); }
    if (worker_.joinable()) worker_.join();
    port_ = 0;
    dispatcher_ = {};
    snapshotBuilder_ = {};
}

void AuthorityLoopbackServer::Run() {
    sockaddr_in peer{};
    socklen_t peerSize = sizeof(peer);
    const int client = accept(listenFd_.load(), reinterpret_cast<sockaddr*>(&peer), &peerSize);
    if (client < 0) {
        if (running_.load()) lastError_.store(AuthorityTransportError::AcceptFailed);
        running_.store(false);
        return;
    }
    clientFd_.store(client);
    const auto closeClient = [this, client]() {
        if (clientFd_.exchange(-1) == client) close(client);
    };
    uint16_t commandCount = 0;
    while (running_.load() && commandCount < kMaxCommandsPerConnection) {
        std::array<uint8_t, 4> lengthBytes{};
        if (!ReceiveAll(client, lengthBytes.data(), lengthBytes.size())) {
            if (running_.load() && commandCount == 0) lastError_.store(AuthorityTransportError::ReceiveFailed);
            break;
        }
        const uint32_t frameLength = (static_cast<uint32_t>(lengthBytes[0]) << 24U) | (static_cast<uint32_t>(lengthBytes[1]) << 16U) | (static_cast<uint32_t>(lengthBytes[2]) << 8U) | lengthBytes[3];
        if (frameLength == 0 || frameLength > AuthorityWireProtocol::kMaxFrameBytes) { lastError_.store(AuthorityTransportError::WireRejected); break; }
        std::vector<uint8_t> frame(frameLength);
        if (!ReceiveAll(client, frame.data(), frame.size())) { if (running_.load()) lastError_.store(AuthorityTransportError::ReceiveFailed); break; }
        AuthorityCommand command{};
        AuthorityWireError wireError{};
        if (!AuthorityWireProtocol::DecodeCommand(frame, command, wireError)) { lastError_.store(AuthorityTransportError::WireRejected); break; }
        const AuthorityDecision decision = dispatcher_(command);
        AuthorityWireSnapshot snapshot{};
        if (!snapshotBuilder_(decision, snapshot)) { lastError_.store(AuthorityTransportError::WireRejected); break; }
        std::vector<uint8_t> response;
        if (!AuthorityWireProtocol::EncodeSnapshot(snapshot, response, wireError) || response.size() > UINT32_MAX) { lastError_.store(AuthorityTransportError::WireRejected); break; }
        const uint32_t responseLength = static_cast<uint32_t>(response.size());
        const std::array<uint8_t, 4> responseHeader{static_cast<uint8_t>(responseLength >> 24U), static_cast<uint8_t>(responseLength >> 16U), static_cast<uint8_t>(responseLength >> 8U), static_cast<uint8_t>(responseLength)};
        if (!SendAll(client, responseHeader.data(), responseHeader.size()) || !SendAll(client, response.data(), response.size())) { lastError_.store(AuthorityTransportError::SendFailed); break; }
        ++commandCount;
    }
    closeClient();
    running_.store(false);
}

} // namespace NeoEngine
