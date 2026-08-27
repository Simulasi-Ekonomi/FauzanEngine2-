#include "FarmAuthoritativeSessionLoopback.h"

#include <utility>

namespace NeoEngine {

bool FarmAuthoritativeSessionLoopback::Start(FarmAuthoritativeSessionHost& host, AuthorityLoopbackServer& transport,
                                             const FarmSessionPrincipal& serverAuthenticatedPrincipal, uint64_t serverTick) {
    Stop();
    if (!host.IsReady() || transport.IsRunning() || serverTick == 0U) {
        lastError_.store(FarmAuthoritativeSessionLoopbackError::InvalidConfiguration);
        return false;
    }
    uint64_t candidateSessionHandle = 0U;
    if (!host.Authenticate(serverAuthenticatedPrincipal, candidateSessionHandle) || candidateSessionHandle == 0U) {
        lastError_.store(FarmAuthoritativeSessionLoopbackError::AuthenticationRejected);
        return false;
    }

    host_ = &host;
    transport_ = &transport;
    principal_ = serverAuthenticatedPrincipal;
    sessionHandle_ = candidateSessionHandle;
    serverTick_ = serverTick;
    hasAcceptedReceipt_ = false;
    ready_ = true;
    lastError_.store(FarmAuthoritativeSessionLoopbackError::None);
    if (!transport.Start(
            [this](const AuthorityCommand& command) { return Dispatch(command); },
            [this](const AuthorityDecision& decision, AuthorityWireSnapshot& snapshot) { return BuildSnapshot(decision, snapshot); })) {
        Reset();
        lastError_.store(FarmAuthoritativeSessionLoopbackError::TransportStartRejected);
        return false;
    }
    return true;
}

void FarmAuthoritativeSessionLoopback::Stop() {
    if (transport_ != nullptr) transport_->Stop();
    Reset();
}

bool FarmAuthoritativeSessionLoopback::IsRunning() const {
    return ready_ && transport_ != nullptr && transport_->IsRunning();
}

uint16_t FarmAuthoritativeSessionLoopback::Port() const {
    return IsRunning() ? transport_->Port() : 0U;
}

AuthorityDecision FarmAuthoritativeSessionLoopback::Dispatch(const AuthorityCommand& wireCommand) {
    if (!ready_ || host_ == nullptr || wireCommand.playerId != principal_.playerId || wireCommand.sessionId != principal_.sessionId) {
        lastError_.store(FarmAuthoritativeSessionLoopbackError::WirePrincipalRejected);
        return {AuthorityError::Unauthenticated, 0U, false};
    }
    FarmSessionCommand command{wireCommand.playerId, wireCommand.commandId, wireCommand.kind,
                               wireCommand.clientSequence, wireCommand.clientTick, wireCommand.payload};
    FarmAuthoritativeCommandReceipt candidate{};
    if (!host_->Submit(sessionHandle_, command, serverTick_, candidate) || !candidate.decision.Accepted()) {
        lastError_.store(FarmAuthoritativeSessionLoopbackError::CommandRejected);
        return {AuthorityError::HandlerRejected, 0U, false};
    }
    if (candidate.snapshot.version != FarmAuthoritativeSnapshotReceipt::kVersion ||
        candidate.snapshot.authoritativeRevision == 0U || candidate.snapshot.worldBytes.empty() ||
        candidate.decision.authoritativeRevision != candidate.snapshot.authoritativeRevision) {
        lastError_.store(FarmAuthoritativeSessionLoopbackError::SnapshotRejected);
        return {AuthorityError::HandlerRejected, 0U, false};
    }
    lastAcceptedReceipt_ = std::move(candidate);
    hasAcceptedReceipt_ = true;
    lastError_.store(FarmAuthoritativeSessionLoopbackError::None);
    return lastAcceptedReceipt_.decision;
}

bool FarmAuthoritativeSessionLoopback::BuildSnapshot(const AuthorityDecision& decision, AuthorityWireSnapshot& snapshot) {
    snapshot = {};
    if (!ready_ || !decision.Accepted() || !hasAcceptedReceipt_ ||
        lastAcceptedReceipt_.snapshot.authoritativeRevision != decision.authoritativeRevision ||
        lastAcceptedReceipt_.snapshot.worldBytes.empty()) {
        if (decision.Accepted()) lastError_.store(FarmAuthoritativeSessionLoopbackError::SnapshotRejected);
        return false;
    }
    snapshot = {lastAcceptedReceipt_.snapshot.authoritativeRevision, lastAcceptedReceipt_.snapshot.worldBytes};
    return true;
}

void FarmAuthoritativeSessionLoopback::Reset() {
    host_ = nullptr;
    transport_ = nullptr;
    principal_ = {};
    sessionHandle_ = 0U;
    serverTick_ = 0U;
    lastAcceptedReceipt_ = {};
    hasAcceptedReceipt_ = false;
    ready_ = false;
}

} // namespace NeoEngine
