#include "Systems/AuthoritativeCommandGate.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>
#include <string>

int main() {
    using namespace NeoEngine;
    constexpr uint16_t kPlayers = 10000;
    TrustSafetySystem trust;
    AuthoritativeCommandGate gate;
    AuthorityConfig config{};
    config.maxPlayers = kPlayers;
    config.maxCommandsPerWindow = 2;
    config.windowTicks = 60;
    config.maxClientTickLead = 5;
    config.maxClientTickLag = 60;
    if (!gate.Initialize(trust, config)) return 1;
    uint32_t handlerCalls = 0;
    for (uint16_t index = 0; index < kPlayers; ++index) {
        const std::string playerId = "scale-player-" + std::to_string(index);
        const std::string sessionId = "scale-session-" + std::to_string(index);
        const std::string commandId = "scale-command-" + std::to_string(index);
        if (!gate.BindSession(playerId, sessionId)) return 1;
        const AuthorityDecision decision = gate.Submit({playerId, sessionId, commandId, "game.action", 1, 1, {}}, 1, [&handlerCalls](const AuthorityCommand&, uint64_t) {
            ++handlerCalls;
            return true;
        });
        if (!decision.Accepted() || decision.replayed) return 1;
    }
    for (uint16_t index = 0; index < kPlayers; ++index) {
        const std::string playerId = "scale-player-" + std::to_string(index);
        const std::string sessionId = "scale-session-" + std::to_string(index);
        const std::string commandId = "scale-command-" + std::to_string(index);
        const AuthorityDecision replay = gate.Submit({playerId, sessionId, commandId, "game.action", 1, 1, {}}, 1, [&handlerCalls](const AuthorityCommand&, uint64_t) {
            ++handlerCalls;
            return true;
        });
        if (!replay.Accepted() || !replay.replayed || replay.authoritativeRevision != static_cast<uint64_t>(index) + 1U) return 1;
    }
    if (handlerCalls != kPlayers || gate.BoundPlayerCount() != kPlayers || gate.AuthoritativeRevision() != kPlayers) return 1;
    std::printf("AUTHORITY_SCALE_SMOKE_OK players=%u revision=%llu handlerCalls=%u replays=%u\n", kPlayers, static_cast<unsigned long long>(gate.AuthoritativeRevision()), handlerCalls, kPlayers);
    return 0;
}
