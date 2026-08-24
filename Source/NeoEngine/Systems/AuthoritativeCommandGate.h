#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace NeoEngine {

class TrustSafetySystem;

enum class AuthorityError : uint8_t {
    None,
    NotInitialized,
    InvalidConfiguration,
    InvalidCommand,
    PlayerCapacity,
    Unauthenticated,
    Banned,
    Duplicate,
    OutOfOrder,
    RateLimited,
    HandlerRejected,
    CorruptPersistence,
};

struct AuthorityConfig {
    uint16_t maxPlayers = 10000;
    uint16_t maxCommandsPerWindow = 30;
    uint32_t windowTicks = 60;
    uint32_t maxClientTickLead = 5;
    uint32_t maxClientTickLag = 600;
};

struct AuthorityCommand {
    std::string playerId;
    std::string sessionId;
    std::string commandId;
    std::string kind;
    uint64_t clientSequence = 0;
    uint64_t clientTick = 0;
    std::vector<uint8_t> payload;
};

struct AuthorityDecision {
    AuthorityError error = AuthorityError::None;
    uint64_t authoritativeRevision = 0;
    bool replayed = false;

    [[nodiscard]] bool Accepted() const { return error == AuthorityError::None; }
};

class AuthoritativeCommandGate {
public:
    static constexpr uint16_t kAbsoluteMaxPlayers = 10000;
    static constexpr uint16_t kMaxRecentCommandsPerPlayer = 128;
    static constexpr size_t kMaxIdLength = 96;
    static constexpr size_t kMaxCommandKindLength = 96;
    static constexpr size_t kMaxPayloadBytes = 4096;

    using CommandHandler = std::function<bool(const AuthorityCommand&, uint64_t authoritativeRevision)>;

    bool Initialize(TrustSafetySystem& trustSafety, const AuthorityConfig& config = {});
    bool BindSession(const std::string& playerId, const std::string& sessionId);
    AuthorityDecision Submit(const AuthorityCommand& command, uint64_t serverTick, const CommandHandler& handler);
    [[nodiscard]] std::vector<uint8_t> SerializeState() const;
    bool DeserializeState(std::span<const uint8_t> bytes);
    [[nodiscard]] uint64_t AuthoritativeRevision() const { return authoritativeRevision_; }
    [[nodiscard]] uint32_t BoundPlayerCount() const { return static_cast<uint32_t>(players_.size()); }
    [[nodiscard]] AuthorityError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }

private:
    struct RecentCommand {
        std::string id;
        uint64_t revision = 0;
    };
    struct PlayerState {
        std::string playerId;
        std::string sessionId;
        uint64_t lastSequence = 0;
        uint64_t windowStartTick = 0;
        uint16_t commandsInWindow = 0;
        std::vector<RecentCommand> recentCommands;
    };

    static bool IsValidId(const std::string& value, size_t minimumLength);
    static bool IsValidCommand(const AuthorityCommand& command);
    PlayerState* FindPlayer(const std::string& playerId);
    const PlayerState* FindPlayer(const std::string& playerId) const;
    AuthorityDecision Reject(AuthorityError error);

    TrustSafetySystem* trustSafety_ = nullptr;
    AuthorityConfig config_{};
    std::vector<PlayerState> players_;
    uint64_t authoritativeRevision_ = 0;
    AuthorityError lastError_ = AuthorityError::None;
    bool initialized_ = false;
};

} // namespace NeoEngine
