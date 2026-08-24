#pragma once

#include "AuthoritativeCommandGate.h"
#include "AuthorityWireProtocol.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>

namespace NeoEngine {

enum class AuthorityTransportError : uint8_t { None, InvalidConfiguration, SocketOpenFailed, BindFailed, ListenFailed, AcceptFailed, ReceiveFailed, WireRejected, SendFailed, NotRunning };

class AuthorityLoopbackServer {
public:
    static constexpr uint16_t kMaxCommandsPerConnection = 32;
    using Handler = AuthoritativeCommandGate::CommandHandler;
    using Dispatcher = std::function<AuthorityDecision(const AuthorityCommand&)>;
    using SnapshotBuilder = std::function<bool(const AuthorityDecision&, AuthorityWireSnapshot&)>;

    AuthorityLoopbackServer() = default;
    ~AuthorityLoopbackServer();

    AuthorityLoopbackServer(const AuthorityLoopbackServer&) = delete;
    AuthorityLoopbackServer& operator=(const AuthorityLoopbackServer&) = delete;

    bool Start(AuthoritativeCommandGate& gate, uint64_t serverTick, Handler handler);
    bool Start(Dispatcher dispatcher, SnapshotBuilder snapshotBuilder);
    void Stop();

    [[nodiscard]] bool IsRunning() const { return running_.load(); }
    [[nodiscard]] uint16_t Port() const { return port_; }
    [[nodiscard]] AuthorityTransportError LastError() const { return lastError_.load(); }

private:
    void Run();

    std::atomic<bool> running_{false};
    std::atomic<AuthorityTransportError> lastError_{AuthorityTransportError::None};
    std::atomic<int> listenFd_{-1};
    std::atomic<int> clientFd_{-1};
    uint16_t port_ = 0;
    Dispatcher dispatcher_;
    SnapshotBuilder snapshotBuilder_;
    std::thread worker_;
};

} // namespace NeoEngine
