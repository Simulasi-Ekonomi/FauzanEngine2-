#pragma once

#include "FarmAuthoritativeService.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class FarmAuthoritativeSessionError : uint8_t {
    None,
    NotInitialized,
    InvalidConfiguration,
    InvalidPrincipal,
    SessionCapacity,
    UnknownSession,
    StaleSession,
    SubjectSpoofed,
    CommandRejected,
    SnapshotRejected,
};

struct FarmSessionPrincipal {
    std::string playerId;
    std::string sessionId;
};

struct FarmSessionCommand {
    std::string claimedPlayerId;
    std::string commandId;
    std::string kind;
    uint64_t clientSequence = 0U;
    uint64_t clientTick = 0U;
    std::vector<uint8_t> payload;
};

struct FarmAuthoritativeSnapshotReceipt {
    static constexpr uint16_t kVersion = 1U;
    uint16_t version = kVersion;
    uint64_t authoritativeRevision = 0U;
    uint64_t stateHash = 0U;
    std::vector<uint8_t> worldBytes;
};

struct FarmAuthoritativeDeltaReceipt {
    uint64_t baseRevision = 0U;
    uint64_t authoritativeRevision = 0U;
    uint64_t baseStateHash = 0U;
    uint64_t stateHash = 0U;
    bool stateChanged = false;
};

struct FarmAuthoritativeCommandReceipt {
    static constexpr uint16_t kVersion = 1U;
    uint16_t version = kVersion;
    AuthorityDecision decision{};
    FarmAuthoritativeSnapshotReceipt snapshot{};
    FarmAuthoritativeDeltaReceipt delta{};
};

class FarmAuthoritativeSessionHost {
public:
    static constexpr uint16_t kMaxSessions = 64U;

    bool Initialize(FarmAuthoritativeService& authority);
    bool Authenticate(const FarmSessionPrincipal& principal, uint64_t& sessionHandle);
    bool Submit(uint64_t sessionHandle, const FarmSessionCommand& command, uint64_t serverTick, FarmAuthoritativeCommandReceipt& receipt);

    [[nodiscard]] bool IsReady() const { return initialized_; }
    [[nodiscard]] uint32_t SessionCount() const { return static_cast<uint32_t>(sessions_.size()); }
    [[nodiscard]] FarmAuthoritativeSessionError LastError() const { return lastError_; }
    [[nodiscard]] const FarmAuthoritativeCommandReceipt* LastReceipt() const { return hasLastReceipt_ ? &lastReceipt_ : nullptr; }

private:
    struct Session {
        uint64_t handle = 0U;
        FarmSessionPrincipal principal{};
    };

    static bool ValidPrincipal(const FarmSessionPrincipal& principal);
    static uint64_t Hash(const std::vector<uint8_t>& bytes);
    bool Fail(FarmAuthoritativeSessionError error);
    Session* FindSession(uint64_t handle);
    const Session* FindSession(uint64_t handle) const;
    bool Snapshot(FarmAuthoritativeSnapshotReceipt& snapshot) const;

    FarmAuthoritativeService* authority_ = nullptr;
    std::vector<Session> sessions_;
    uint64_t nextSessionHandle_ = 1U;
    FarmAuthoritativeCommandReceipt lastReceipt_{};
    bool hasLastReceipt_ = false;
    FarmAuthoritativeSessionError lastError_ = FarmAuthoritativeSessionError::None;
    bool initialized_ = false;
};

} // namespace NeoEngine
