#pragma once

#include "AuthorityLoopbackServer.h"
#include "FarmAuthoritativeSessionHost.h"

#include <atomic>
#include <cstdint>

namespace NeoEngine {

enum class FarmAuthoritativeSessionLoopbackError : uint8_t {
    None,
    InvalidConfiguration,
    AuthenticationRejected,
    TransportStartRejected,
    WirePrincipalRejected,
    CommandRejected,
    SnapshotRejected,
};

class FarmAuthoritativeSessionLoopback {
public:
    bool Start(FarmAuthoritativeSessionHost& host, AuthorityLoopbackServer& transport,
               const FarmSessionPrincipal& serverAuthenticatedPrincipal, uint64_t serverTick);
    void Stop();

    [[nodiscard]] bool IsRunning() const;
    [[nodiscard]] uint16_t Port() const;
    [[nodiscard]] FarmAuthoritativeSessionLoopbackError LastError() const { return lastError_.load(); }

private:
    AuthorityDecision Dispatch(const AuthorityCommand& wireCommand);
    bool BuildSnapshot(const AuthorityDecision& decision, AuthorityWireSnapshot& snapshot);
    void Reset();

    FarmAuthoritativeSessionHost* host_ = nullptr;
    AuthorityLoopbackServer* transport_ = nullptr;
    FarmSessionPrincipal principal_{};
    uint64_t sessionHandle_ = 0U;
    uint64_t serverTick_ = 0U;
    FarmAuthoritativeCommandReceipt lastAcceptedReceipt_{};
    bool hasAcceptedReceipt_ = false;
    bool ready_ = false;
    std::atomic<FarmAuthoritativeSessionLoopbackError> lastError_{FarmAuthoritativeSessionLoopbackError::None};
};

} // namespace NeoEngine
