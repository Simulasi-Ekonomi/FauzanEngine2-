#include "Systems/AuthoritativeCommandGate.h"
#include "Systems/TrustSafetySystem.h"

#include <cassert>
#include <cstdint>
#include <vector>

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

    assert(gate.Initialize(trust, config));
    assert(gate.BindSession("player-a", "session-a-123"));

    AuthorityCommand command{"player-a", "session-a-123", "command-001", "farm.till", 1, 5, {}};
    const auto rejectHandler = [](const AuthorityCommand&, uint64_t) { return false; };
    const AuthorityDecision rejected = gate.Submit(command, 5, rejectHandler);
    assert(rejected.error == AuthorityError::HandlerRejected);
    assert(gate.AuthoritativeRevision() == 0);

    const auto acceptHandler = [](const AuthorityCommand&, uint64_t revision) { return revision == 1; };
    const AuthorityDecision accepted = gate.Submit(command, 5, acceptHandler);
    assert(accepted.Accepted());
    assert(accepted.authoritativeRevision == 1);

    const std::vector<uint8_t> valid = gate.SerializeState();
    assert(!valid.empty());

    AuthoritativeCommandGate restored;
    assert(restored.Initialize(trust, config));
    assert(restored.DeserializeState(valid));
    assert(restored.AuthoritativeRevision() == 1);
    assert(restored.BoundPlayerCount() == 1);

    std::vector<uint8_t> truncated = valid;
    truncated.pop_back();
    assert(!restored.DeserializeState(truncated));
    assert(restored.LastError() == AuthorityError::CorruptPersistence);
    assert(restored.AuthoritativeRevision() == 1);
    assert(restored.BoundPlayerCount() == 1);

    std::vector<uint8_t> malformed = valid;
    assert(malformed.size() >= 2);
    malformed[0] = 0;
    assert(!restored.DeserializeState(malformed));
    assert(restored.AuthoritativeRevision() == 1);
    assert(restored.BoundPlayerCount() == 1);

    return 0;
}
