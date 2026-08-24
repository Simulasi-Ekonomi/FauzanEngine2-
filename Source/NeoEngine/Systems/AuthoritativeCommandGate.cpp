#include "AuthoritativeCommandGate.h"

#include "TrustSafetySystem.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {
namespace {

constexpr uint32_t kLedgerMagic = 0x48545541U; // AUTH
constexpr uint16_t kLedgerVersion = 1;

template <typename T>
void Append(std::vector<uint8_t>& output, T value) {
    for (size_t byte = 0; byte < sizeof(T); ++byte) output.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (byte * 8U)) & 0xFFU));
}

template <typename T>
bool Read(std::span<const uint8_t> input, size_t& offset, T& value) {
    if (offset + sizeof(T) > input.size()) return false;
    uint64_t raw = 0;
    for (size_t byte = 0; byte < sizeof(T); ++byte) raw |= static_cast<uint64_t>(input[offset + byte]) << (byte * 8U);
    value = static_cast<T>(raw);
    offset += sizeof(T);
    return true;
}

bool ReadString(std::span<const uint8_t> input, size_t& offset, std::string& value) {
    uint16_t length = 0;
    if (!Read(input, offset, length) || length == 0 || offset + length > input.size()) return false;
    value.assign(reinterpret_cast<const char*>(input.data() + offset), length);
    offset += length;
    return true;
}

void AppendString(std::vector<uint8_t>& output, const std::string& value) {
    Append<uint16_t>(output, static_cast<uint16_t>(value.size()));
    output.insert(output.end(), value.begin(), value.end());
}

} // namespace

bool AuthoritativeCommandGate::Initialize(TrustSafetySystem& trustSafety, const AuthorityConfig& config) {
    initialized_ = false;
    trustSafety_ = nullptr;
    players_.clear();
    authoritativeRevision_ = 0;
    lastError_ = AuthorityError::None;
    if (config.maxPlayers == 0 || config.maxPlayers > kAbsoluteMaxPlayers || config.maxCommandsPerWindow == 0 || config.maxCommandsPerWindow > kMaxRecentCommandsPerPlayer || config.windowTicks == 0 || config.maxClientTickLag == 0) {
        lastError_ = AuthorityError::InvalidConfiguration;
        return false;
    }
    config_ = config;
    players_.reserve(config.maxPlayers);
    trustSafety_ = &trustSafety;
    initialized_ = true;
    return true;
}

bool AuthoritativeCommandGate::BindSession(const std::string& playerId, const std::string& sessionId) {
    if (!initialized_) {
        lastError_ = AuthorityError::NotInitialized;
        return false;
    }
    if (!IsValidId(playerId, 1) || !IsValidId(sessionId, 8)) {
        lastError_ = AuthorityError::InvalidCommand;
        return false;
    }
    if (trustSafety_->IsBanned(playerId)) {
        lastError_ = AuthorityError::Banned;
        return false;
    }
    PlayerState* player = FindPlayer(playerId);
    if (player == nullptr) {
        if (players_.size() >= config_.maxPlayers) {
            lastError_ = AuthorityError::PlayerCapacity;
            return false;
        }
        players_.push_back({playerId, sessionId, 0, 0, 0, {}});
    } else {
        player->sessionId = sessionId;
    }
    lastError_ = AuthorityError::None;
    return true;
}

AuthorityDecision AuthoritativeCommandGate::Submit(const AuthorityCommand& command, uint64_t serverTick, const CommandHandler& handler) {
    if (!initialized_) return Reject(AuthorityError::NotInitialized);
    if (!IsValidCommand(command) || !handler) return Reject(AuthorityError::InvalidCommand);
    PlayerState* player = FindPlayer(command.playerId);
    if (player == nullptr || player->sessionId != command.sessionId) return Reject(AuthorityError::Unauthenticated);
    if (trustSafety_->IsBanned(command.playerId)) return Reject(AuthorityError::Banned);
    if ((command.clientTick > serverTick && command.clientTick - serverTick > config_.maxClientTickLead) ||
        (serverTick > command.clientTick && serverTick - command.clientTick > config_.maxClientTickLag)) {
        return Reject(AuthorityError::InvalidCommand);
    }

    const auto duplicate = std::find_if(player->recentCommands.begin(), player->recentCommands.end(), [&command](const RecentCommand& entry) { return entry.id == command.commandId; });
    if (duplicate != player->recentCommands.end()) {
        lastError_ = AuthorityError::None;
        return {AuthorityError::None, duplicate->revision, true};
    }
    if (command.clientSequence != player->lastSequence + 1U) return Reject(AuthorityError::OutOfOrder);
    if (serverTick < player->windowStartTick) return Reject(AuthorityError::InvalidCommand);
    if (serverTick - player->windowStartTick >= config_.windowTicks) {
        player->windowStartTick = serverTick;
        player->commandsInWindow = 0;
    }
    if (player->commandsInWindow >= config_.maxCommandsPerWindow) return Reject(AuthorityError::RateLimited);
    if (authoritativeRevision_ == std::numeric_limits<uint64_t>::max()) return Reject(AuthorityError::InvalidConfiguration);
    const uint64_t nextRevision = authoritativeRevision_ + 1U;
    if (!handler(command, nextRevision)) return Reject(AuthorityError::HandlerRejected);

    authoritativeRevision_ = nextRevision;
    player->lastSequence = command.clientSequence;
    ++player->commandsInWindow;
    if (player->recentCommands.size() == kMaxRecentCommandsPerPlayer) player->recentCommands.erase(player->recentCommands.begin());
    player->recentCommands.push_back({command.commandId, nextRevision});
    lastError_ = AuthorityError::None;
    return {AuthorityError::None, nextRevision, false};
}

std::vector<uint8_t> AuthoritativeCommandGate::SerializeState() const {
    if (!initialized_) return {};
    std::vector<uint8_t> output;
    output.reserve(32 + players_.size() * 256U);
    Append<uint32_t>(output, kLedgerMagic);
    Append<uint16_t>(output, kLedgerVersion);
    Append<uint16_t>(output, config_.maxPlayers);
    Append<uint16_t>(output, config_.maxCommandsPerWindow);
    Append<uint32_t>(output, config_.windowTicks);
    Append<uint32_t>(output, config_.maxClientTickLead);
    Append<uint32_t>(output, config_.maxClientTickLag);
    Append<uint64_t>(output, authoritativeRevision_);
    Append<uint16_t>(output, static_cast<uint16_t>(players_.size()));
    for (const PlayerState& player : players_) {
        AppendString(output, player.playerId);
        Append<uint64_t>(output, player.lastSequence);
        Append<uint64_t>(output, player.windowStartTick);
        Append<uint16_t>(output, player.commandsInWindow);
        Append<uint16_t>(output, static_cast<uint16_t>(player.recentCommands.size()));
        for (const RecentCommand& command : player.recentCommands) {
            AppendString(output, command.id);
            Append<uint64_t>(output, command.revision);
        }
    }
    return output;
}

bool AuthoritativeCommandGate::DeserializeState(std::span<const uint8_t> bytes) {
    if (!initialized_) {
        lastError_ = AuthorityError::NotInitialized;
        return false;
    }
    size_t offset = 0;
    uint32_t magic = 0;
    uint16_t version = 0;
    AuthorityConfig storedConfig{};
    uint64_t revision = 0;
    uint16_t playerCount = 0;
    if (!Read(bytes, offset, magic) || !Read(bytes, offset, version) || !Read(bytes, offset, storedConfig.maxPlayers) || !Read(bytes, offset, storedConfig.maxCommandsPerWindow) ||
        !Read(bytes, offset, storedConfig.windowTicks) || !Read(bytes, offset, storedConfig.maxClientTickLead) || !Read(bytes, offset, storedConfig.maxClientTickLag) ||
        !Read(bytes, offset, revision) || !Read(bytes, offset, playerCount) || magic != kLedgerMagic || version != kLedgerVersion ||
        storedConfig.maxPlayers != config_.maxPlayers || storedConfig.maxCommandsPerWindow != config_.maxCommandsPerWindow || storedConfig.windowTicks != config_.windowTicks ||
        storedConfig.maxClientTickLead != config_.maxClientTickLead || storedConfig.maxClientTickLag != config_.maxClientTickLag || playerCount > config_.maxPlayers) {
        lastError_ = AuthorityError::CorruptPersistence;
        return false;
    }
    std::vector<PlayerState> restored;
    restored.reserve(playerCount);
    for (uint16_t playerIndex = 0; playerIndex < playerCount; ++playerIndex) {
        PlayerState player{};
        uint16_t recentCount = 0;
        if (!ReadString(bytes, offset, player.playerId) || !IsValidId(player.playerId, 1) || !Read(bytes, offset, player.lastSequence) || !Read(bytes, offset, player.windowStartTick) ||
            !Read(bytes, offset, player.commandsInWindow) || !Read(bytes, offset, recentCount) || player.commandsInWindow > config_.maxCommandsPerWindow || recentCount > kMaxRecentCommandsPerPlayer ||
            std::any_of(restored.begin(), restored.end(), [&player](const PlayerState& existing) { return existing.playerId == player.playerId; })) {
            lastError_ = AuthorityError::CorruptPersistence;
            return false;
        }
        player.recentCommands.reserve(recentCount);
        for (uint16_t commandIndex = 0; commandIndex < recentCount; ++commandIndex) {
            RecentCommand command{};
            if (!ReadString(bytes, offset, command.id) || !IsValidId(command.id, 8) || !Read(bytes, offset, command.revision) || command.revision == 0 || command.revision > revision ||
                std::any_of(player.recentCommands.begin(), player.recentCommands.end(), [&command](const RecentCommand& existing) { return existing.id == command.id; })) {
                lastError_ = AuthorityError::CorruptPersistence;
                return false;
            }
            player.recentCommands.push_back(std::move(command));
        }
        player.sessionId.clear(); // Session credentials are deliberately not persisted.
        restored.push_back(std::move(player));
    }
    if (offset != bytes.size()) {
        lastError_ = AuthorityError::CorruptPersistence;
        return false;
    }
    players_ = std::move(restored);
    authoritativeRevision_ = revision;
    lastError_ = AuthorityError::None;
    return true;
}

bool AuthoritativeCommandGate::IsValidId(const std::string& value, size_t minimumLength) {
    if (value.size() < minimumLength || value.size() > kMaxIdLength) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) { return c >= 0x21U && c <= 0x7EU; });
}

bool AuthoritativeCommandGate::IsValidCommand(const AuthorityCommand& command) {
    return IsValidId(command.playerId, 1) && IsValidId(command.sessionId, 8) && IsValidId(command.commandId, 8) &&
           command.kind.size() >= 3 && command.kind.size() <= kMaxCommandKindLength && command.clientSequence != 0 &&
           command.payload.size() <= kMaxPayloadBytes;
}

AuthoritativeCommandGate::PlayerState* AuthoritativeCommandGate::FindPlayer(const std::string& playerId) {
    const auto player = std::find_if(players_.begin(), players_.end(), [&playerId](const PlayerState& state) { return state.playerId == playerId; });
    return player == players_.end() ? nullptr : &*player;
}

const AuthoritativeCommandGate::PlayerState* AuthoritativeCommandGate::FindPlayer(const std::string& playerId) const {
    const auto player = std::find_if(players_.begin(), players_.end(), [&playerId](const PlayerState& state) { return state.playerId == playerId; });
    return player == players_.end() ? nullptr : &*player;
}

AuthorityDecision AuthoritativeCommandGate::Reject(AuthorityError error) {
    lastError_ = error;
    return {error, authoritativeRevision_, false};
}

} // namespace NeoEngine
