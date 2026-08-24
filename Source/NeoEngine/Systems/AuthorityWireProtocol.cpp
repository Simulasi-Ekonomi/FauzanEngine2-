#include "AuthorityWireProtocol.h"

#include <limits>

namespace NeoEngine {
namespace {

constexpr uint32_t kMagic = 0x52485741U; // AWHR
constexpr uint8_t kCommandType = 1;
constexpr uint8_t kSnapshotType = 2;

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

bool AppendString(std::vector<uint8_t>& output, const std::string& value) {
    if (value.empty() || value.size() > std::numeric_limits<uint16_t>::max()) return false;
    Append<uint16_t>(output, static_cast<uint16_t>(value.size()));
    output.insert(output.end(), value.begin(), value.end());
    return true;
}

bool ReadString(std::span<const uint8_t> input, size_t& offset, std::string& value) {
    uint16_t length = 0;
    if (!Read(input, offset, length) || length == 0 || offset + length > input.size()) return false;
    value.assign(reinterpret_cast<const char*>(input.data() + offset), length);
    offset += length;
    return true;
}

bool ValidCommand(const AuthorityCommand& command) {
    return command.playerId.size() <= AuthoritativeCommandGate::kMaxIdLength && command.sessionId.size() <= AuthoritativeCommandGate::kMaxIdLength &&
           command.commandId.size() <= AuthoritativeCommandGate::kMaxIdLength && command.kind.size() <= AuthoritativeCommandGate::kMaxCommandKindLength &&
           !command.playerId.empty() && !command.sessionId.empty() && !command.commandId.empty() && !command.kind.empty() && command.payload.size() <= AuthoritativeCommandGate::kMaxPayloadBytes;
}

bool ReadPrefix(std::span<const uint8_t> frame, uint8_t expectedType, size_t& offset) {
    uint32_t magic = 0;
    uint16_t version = 0;
    uint8_t type = 0;
    return Read(frame, offset, magic) && Read(frame, offset, version) && Read(frame, offset, type) && magic == kMagic && version == AuthorityWireProtocol::kVersion && type == expectedType;
}

void WritePrefix(std::vector<uint8_t>& frame, uint8_t type) {
    Append<uint32_t>(frame, kMagic);
    Append<uint16_t>(frame, AuthorityWireProtocol::kVersion);
    Append<uint8_t>(frame, type);
}

} // namespace

bool AuthorityWireProtocol::EncodeCommand(const AuthorityCommand& command, std::vector<uint8_t>& frame, AuthorityWireError& error) {
    frame.clear();
    error = AuthorityWireError::None;
    if (!ValidCommand(command)) { error = AuthorityWireError::InvalidFrame; return false; }
    frame.reserve(256 + command.payload.size());
    WritePrefix(frame, kCommandType);
    if (!AppendString(frame, command.playerId) || !AppendString(frame, command.sessionId) || !AppendString(frame, command.commandId) || !AppendString(frame, command.kind)) {
        frame.clear(); error = AuthorityWireError::InvalidFrame; return false;
    }
    Append<uint64_t>(frame, command.clientSequence);
    Append<uint64_t>(frame, command.clientTick);
    Append<uint16_t>(frame, static_cast<uint16_t>(command.payload.size()));
    frame.insert(frame.end(), command.payload.begin(), command.payload.end());
    if (frame.size() > kMaxFrameBytes) { frame.clear(); error = AuthorityWireError::SizeLimit; return false; }
    return true;
}

bool AuthorityWireProtocol::DecodeCommand(std::span<const uint8_t> frame, AuthorityCommand& command, AuthorityWireError& error) {
    command = {};
    error = AuthorityWireError::None;
    if (frame.size() > kMaxFrameBytes) { error = AuthorityWireError::SizeLimit; return false; }
    size_t offset = 0;
    uint16_t payloadLength = 0;
    if (!ReadPrefix(frame, kCommandType, offset) || !ReadString(frame, offset, command.playerId) || !ReadString(frame, offset, command.sessionId) || !ReadString(frame, offset, command.commandId) || !ReadString(frame, offset, command.kind) ||
        !Read(frame, offset, command.clientSequence) || !Read(frame, offset, command.clientTick) || !Read(frame, offset, payloadLength) || payloadLength > AuthoritativeCommandGate::kMaxPayloadBytes || offset + payloadLength != frame.size()) {
        error = AuthorityWireError::CorruptFrame;
        command = {};
        return false;
    }
    command.payload.assign(frame.begin() + static_cast<std::ptrdiff_t>(offset), frame.end());
    if (!ValidCommand(command)) { error = AuthorityWireError::InvalidFrame; command = {}; return false; }
    return true;
}

bool AuthorityWireProtocol::EncodeSnapshot(const AuthorityWireSnapshot& snapshot, std::vector<uint8_t>& frame, AuthorityWireError& error) {
    frame.clear();
    error = AuthorityWireError::None;
    if (snapshot.revision == 0 || snapshot.state.empty() || snapshot.state.size() > kMaxSnapshotBytes) { error = AuthorityWireError::InvalidFrame; return false; }
    frame.reserve(16 + snapshot.state.size());
    WritePrefix(frame, kSnapshotType);
    Append<uint64_t>(frame, snapshot.revision);
    Append<uint32_t>(frame, static_cast<uint32_t>(snapshot.state.size()));
    frame.insert(frame.end(), snapshot.state.begin(), snapshot.state.end());
    if (frame.size() > kMaxSnapshotBytes + 32U) { frame.clear(); error = AuthorityWireError::SizeLimit; return false; }
    return true;
}

bool AuthorityWireProtocol::DecodeSnapshot(std::span<const uint8_t> frame, AuthorityWireSnapshot& snapshot, AuthorityWireError& error) {
    snapshot = {};
    error = AuthorityWireError::None;
    if (frame.size() > kMaxSnapshotBytes + 32U) { error = AuthorityWireError::SizeLimit; return false; }
    size_t offset = 0;
    uint32_t stateLength = 0;
    if (!ReadPrefix(frame, kSnapshotType, offset) || !Read(frame, offset, snapshot.revision) || !Read(frame, offset, stateLength) || snapshot.revision == 0 || stateLength == 0 || stateLength > kMaxSnapshotBytes || offset + stateLength != frame.size()) {
        error = AuthorityWireError::CorruptFrame;
        snapshot = {};
        return false;
    }
    snapshot.state.assign(frame.begin() + static_cast<std::ptrdiff_t>(offset), frame.end());
    return true;
}

} // namespace NeoEngine
