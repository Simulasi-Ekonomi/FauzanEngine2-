#include "FarmAuthoritativeSessionHost.h"

#include <algorithm>

namespace NeoEngine {
namespace {
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;
}

bool FarmAuthoritativeSessionHost::Initialize(FarmAuthoritativeService& authority) {
    authority_ = nullptr;
    sessions_.clear();
    nextSessionHandle_ = 1U;
    lastReceipt_ = {};
    hasLastReceipt_ = false;
    lastError_ = FarmAuthoritativeSessionError::None;
    initialized_ = false;
    if (!authority.IsReady()) return Fail(FarmAuthoritativeSessionError::InvalidConfiguration);
    authority_ = &authority;
    sessions_.reserve(kMaxSessions);
    initialized_ = true;
    return true;
}

bool FarmAuthoritativeSessionHost::Authenticate(const FarmSessionPrincipal& principal, uint64_t& sessionHandle) {
    sessionHandle = 0U;
    if (!initialized_ || authority_ == nullptr) return Fail(FarmAuthoritativeSessionError::NotInitialized);
    if (!ValidPrincipal(principal)) return Fail(FarmAuthoritativeSessionError::InvalidPrincipal);
    Session* session = nullptr;
    for (Session& candidate : sessions_) {
        if (candidate.principal.playerId == principal.playerId) {
            session = &candidate;
            break;
        }
    }
    if (session == nullptr) {
        if (sessions_.size() >= kMaxSessions || nextSessionHandle_ == 0U) return Fail(FarmAuthoritativeSessionError::SessionCapacity);
        sessions_.push_back({nextSessionHandle_++, principal});
        session = &sessions_.back();
    } else {
        if (nextSessionHandle_ == 0U) return Fail(FarmAuthoritativeSessionError::SessionCapacity);
        session->handle = nextSessionHandle_++;
        session->principal = principal;
    }
    if (!authority_->BindSession(principal.playerId, principal.sessionId)) return Fail(FarmAuthoritativeSessionError::InvalidPrincipal);
    sessionHandle = session->handle;
    lastError_ = FarmAuthoritativeSessionError::None;
    return true;
}

bool FarmAuthoritativeSessionHost::Submit(uint64_t sessionHandle, const FarmSessionCommand& command, uint64_t serverTick, FarmAuthoritativeCommandReceipt& receipt) {
    receipt = {};
    if (!initialized_ || authority_ == nullptr) return Fail(FarmAuthoritativeSessionError::NotInitialized);
    const Session* session = FindSession(sessionHandle);
    if (session == nullptr) return Fail(FarmAuthoritativeSessionError::UnknownSession);
    if (!ValidPrincipal(session->principal)) return Fail(FarmAuthoritativeSessionError::StaleSession);
    if (command.claimedPlayerId != session->principal.playerId) return Fail(FarmAuthoritativeSessionError::SubjectSpoofed);
    FarmAuthoritativeSnapshotReceipt before{};
    if (!Snapshot(before)) return Fail(FarmAuthoritativeSessionError::SnapshotRejected);
    const AuthorityCommand internal{session->principal.playerId, session->principal.sessionId, command.commandId, command.kind, command.clientSequence, command.clientTick, command.payload};
    const AuthorityDecision decision = authority_->Submit(internal, serverTick);
    if (!decision.Accepted()) return Fail(FarmAuthoritativeSessionError::CommandRejected);
    FarmAuthoritativeSnapshotReceipt after{};
    if (!Snapshot(after)) return Fail(FarmAuthoritativeSessionError::SnapshotRejected);
    const FarmAuthoritativeCommandReceipt candidate{FarmAuthoritativeCommandReceipt::kVersion, decision, after,
        {before.authoritativeRevision, after.authoritativeRevision, before.stateHash, after.stateHash, !decision.replayed && before.stateHash != after.stateHash}};
    receipt = candidate;
    lastReceipt_ = candidate;
    hasLastReceipt_ = true;
    lastError_ = FarmAuthoritativeSessionError::None;
    return true;
}

bool FarmAuthoritativeSessionHost::ValidPrincipal(const FarmSessionPrincipal& principal) {
    const auto valid = [](const std::string& value, size_t minimumLength) {
        return value.size() >= minimumLength && value.size() <= AuthoritativeCommandGate::kMaxIdLength &&
               std::all_of(value.begin(), value.end(), [](unsigned char ch) { return ch >= 0x21U && ch <= 0x7EU; });
    };
    return valid(principal.playerId, 1U) && valid(principal.sessionId, 8U);
}

uint64_t FarmAuthoritativeSessionHost::Hash(const std::vector<uint8_t>& bytes) {
    uint64_t hash = kHashOffset;
    for (uint8_t byte : bytes) {
        hash ^= byte;
        hash *= kHashPrime;
    }
    return hash;
}

bool FarmAuthoritativeSessionHost::Fail(FarmAuthoritativeSessionError error) {
    lastError_ = error;
    return false;
}

FarmAuthoritativeSessionHost::Session* FarmAuthoritativeSessionHost::FindSession(uint64_t handle) {
    const auto found = std::find_if(sessions_.begin(), sessions_.end(), [handle](const Session& value) { return value.handle == handle; });
    return found == sessions_.end() ? nullptr : &*found;
}

const FarmAuthoritativeSessionHost::Session* FarmAuthoritativeSessionHost::FindSession(uint64_t handle) const {
    const auto found = std::find_if(sessions_.begin(), sessions_.end(), [handle](const Session& value) { return value.handle == handle; });
    return found == sessions_.end() ? nullptr : &*found;
}

bool FarmAuthoritativeSessionHost::Snapshot(FarmAuthoritativeSnapshotReceipt& snapshot) const {
    snapshot = {};
    if (!initialized_ || authority_ == nullptr) return false;
    FarmAuthoritySnapshot authoritySnapshot{};
    if (!authority_->BuildSnapshot(authoritySnapshot)) {
        if (authority_->Revision() != 0U) return false;
        snapshot = {FarmAuthoritativeSnapshotReceipt::kVersion, 0U, 0U, {}};
        return true;
    }
    if (authoritySnapshot.revision == 0U || authoritySnapshot.worldBytes.empty()) return false;
    snapshot = {FarmAuthoritativeSnapshotReceipt::kVersion, authoritySnapshot.revision, Hash(authoritySnapshot.worldBytes), std::move(authoritySnapshot.worldBytes)};
    return true;
}

} // namespace NeoEngine
