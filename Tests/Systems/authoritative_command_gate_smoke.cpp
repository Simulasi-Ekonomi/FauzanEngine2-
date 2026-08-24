#include "Systems/AuthoritativeCommandGate.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    TrustSafetySystem trust;
    AuthoritativeCommandGate gate;
    AuthorityConfig config{};
    config.maxPlayers = 2;
    config.maxCommandsPerWindow = 2;
    config.windowTicks = 10;
    config.maxClientTickLead = 5;
    config.maxClientTickLag = 50;
    if (!gate.Initialize(trust, config) || !gate.BindSession("player-a", "session-a-123") || gate.BindSession("player-a", "short")) return 1;

    uint32_t handlerCalls = 0;
    const auto handler = [&handlerCalls](const AuthorityCommand& command, uint64_t revision) {
        ++handlerCalls;
        return command.kind == "farm.till" && revision == handlerCalls;
    };
    AuthorityCommand first{"player-a", "session-a-123", "command-001", "farm.till", 1, 5, {1, 2, 3}};
    const AuthorityDecision accepted = gate.Submit(first, 5, handler);
    const AuthorityDecision replay = gate.Submit(first, 5, handler);
    AuthorityCommand skipped{"player-a", "session-a-123", "command-003", "farm.till", 3, 5, {}};
    const AuthorityDecision outOfOrder = gate.Submit(skipped, 5, handler);
    AuthorityCommand future{"player-a", "session-a-123", "command-future", "farm.till", 2, 99, {}};
    const AuthorityDecision futureRejected = gate.Submit(future, 5, handler);
    AuthorityCommand second{"player-a", "session-a-123", "command-002", "farm.till", 2, 5, {}};
    const AuthorityDecision acceptedSecond = gate.Submit(second, 5, handler);
    AuthorityCommand rateLimited{"player-a", "session-a-123", "command-004", "farm.till", 3, 5, {}};
    const AuthorityDecision rateDecision = gate.Submit(rateLimited, 5, handler);
    AuthorityCommand nextWindow{"player-a", "session-a-123", "command-004", "farm.till", 3, 15, {}};
    const AuthorityDecision acceptedNextWindow = gate.Submit(nextWindow, 15, handler);
    AuthorityCommand wrongSession{"player-a", "session-b-123", "command-005", "farm.till", 4, 15, {}};
    const AuthorityDecision unauthenticated = gate.Submit(wrongSession, 15, handler);
    if (!gate.BindSession("player-a", "session-b-123")) return 1;
    AuthorityCommand replayAfterRotation{"player-a", "session-b-123", "command-004", "farm.till", 3, 15, {}};
    const AuthorityDecision replayAfterRotationDecision = gate.Submit(replayAfterRotation, 15, handler);
    AuthorityCommand acceptedAfterRotation{"player-a", "session-b-123", "command-005", "farm.till", 4, 15, {}};
    const AuthorityDecision acceptedAfterRotationDecision = gate.Submit(acceptedAfterRotation, 15, handler);
    const std::vector<uint8_t> ledgerBytes = gate.SerializeState();
    AuthoritativeCommandGate restoredGate;
    if (ledgerBytes.empty() || !restoredGate.Initialize(trust, config) || !restoredGate.DeserializeState(ledgerBytes) || restoredGate.BoundPlayerCount() != 1 || restoredGate.AuthoritativeRevision() != 4) return 1;
    const AuthorityDecision missingSessionAfterRestore = restoredGate.Submit({"player-a", "session-b-123", "command-005", "farm.till", 4, 15, {}}, 15, handler);
    if (!restoredGate.BindSession("player-a", "session-c-123")) return 1;
    const AuthorityDecision replayAfterRestore = restoredGate.Submit({"player-a", "session-c-123", "command-005", "farm.till", 4, 15, {}}, 15, handler);
    const AuthorityDecision acceptedAfterRestore = restoredGate.Submit({"player-a", "session-c-123", "command-006", "farm.till", 5, 25, {}}, 25, handler);
    if (!trust.Report("player-a", "fraud-a", NeoEngine::FraudSignal::LedgerMismatch) || !trust.Report("player-a", "fraud-b", NeoEngine::FraudSignal::LedgerMismatch)) return 1;
    AuthorityCommand banned{"player-a", "session-b-123", "command-006", "farm.till", 5, 16, {}};
    const AuthorityDecision banDecision = gate.Submit(banned, 16, handler);
    if (!accepted.Accepted() || accepted.authoritativeRevision != 1 || accepted.replayed || !replay.Accepted() || !replay.replayed || replay.authoritativeRevision != 1 ||
        outOfOrder.error != AuthorityError::OutOfOrder || futureRejected.error != AuthorityError::InvalidCommand || !acceptedSecond.Accepted() || acceptedSecond.authoritativeRevision != 2 || rateDecision.error != AuthorityError::RateLimited ||
        !acceptedNextWindow.Accepted() || acceptedNextWindow.authoritativeRevision != 3 || unauthenticated.error != AuthorityError::Unauthenticated ||
        !replayAfterRotationDecision.Accepted() || !replayAfterRotationDecision.replayed || replayAfterRotationDecision.authoritativeRevision != 3 ||
        !acceptedAfterRotationDecision.Accepted() || acceptedAfterRotationDecision.authoritativeRevision != 4 || missingSessionAfterRestore.error != AuthorityError::Unauthenticated ||
        !replayAfterRestore.Accepted() || !replayAfterRestore.replayed || replayAfterRestore.authoritativeRevision != 4 || !acceptedAfterRestore.Accepted() || acceptedAfterRestore.authoritativeRevision != 5 ||
        banDecision.error != AuthorityError::Banned || handlerCalls != 5 || gate.AuthoritativeRevision() != 4 || gate.BoundPlayerCount() != 1) {
        return 1;
    }
    std::printf("AUTHORITATIVE_COMMAND_GATE_SMOKE_OK revision=%llu restoredRevision=%llu handlerCalls=%u replay=1 rotationSafe=1 persistenceNoSession=1 rateLimited=1 banned=1\n", static_cast<unsigned long long>(gate.AuthoritativeRevision()), static_cast<unsigned long long>(restoredGate.AuthoritativeRevision()), handlerCalls);
    return 0;
}
