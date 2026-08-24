#include "Systems/AuthorityWireProtocol.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    AuthorityCommand original{"wire-player", "wire-session-123", "wire-command-001", "farm.till", 7, 11, {2, 0, 3, 0}};
    std::vector<uint8_t> commandFrame;
    AuthorityWireError error = AuthorityWireError::None;
    AuthorityCommand decoded;
    if (!AuthorityWireProtocol::EncodeCommand(original, commandFrame, error) || !AuthorityWireProtocol::DecodeCommand(commandFrame, decoded, error) ||
        decoded.playerId != original.playerId || decoded.sessionId != original.sessionId || decoded.commandId != original.commandId || decoded.kind != original.kind ||
        decoded.clientSequence != 7 || decoded.clientTick != 11 || decoded.payload != original.payload) return 1;
    commandFrame.pop_back();
    if (AuthorityWireProtocol::DecodeCommand(commandFrame, decoded, error) || error != AuthorityWireError::CorruptFrame) return 1;

    AuthorityWireSnapshot originalSnapshot{9, {1, 2, 3, 4, 5}};
    std::vector<uint8_t> snapshotFrame;
    AuthorityWireSnapshot decodedSnapshot;
    if (!AuthorityWireProtocol::EncodeSnapshot(originalSnapshot, snapshotFrame, error) || !AuthorityWireProtocol::DecodeSnapshot(snapshotFrame, decodedSnapshot, error) ||
        decodedSnapshot.revision != 9 || decodedSnapshot.state != originalSnapshot.state) return 1;
    snapshotFrame.push_back(0);
    if (AuthorityWireProtocol::DecodeSnapshot(snapshotFrame, decodedSnapshot, error) || error != AuthorityWireError::CorruptFrame) return 1;
    std::printf("AUTHORITY_WIRE_PROTOCOL_SMOKE_OK commandBytes=%zu snapshotBytes=%zu\n", commandFrame.size(), snapshotFrame.size() - 1U);
    return 0;
}
