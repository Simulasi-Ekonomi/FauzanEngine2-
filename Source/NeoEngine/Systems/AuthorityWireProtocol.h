#pragma once

#include "AuthoritativeCommandGate.h"

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

enum class AuthorityWireError : uint8_t { None, InvalidFrame, UnsupportedVersion, SizeLimit, CorruptFrame };

struct AuthorityWireSnapshot {
    uint64_t revision = 0;
    std::vector<uint8_t> state;
};

class AuthorityWireProtocol {
public:
    static constexpr uint16_t kVersion = 1;
    static constexpr size_t kMaxFrameBytes = 8192;
    static constexpr size_t kMaxSnapshotBytes = 65536;

    static bool EncodeCommand(const AuthorityCommand& command, std::vector<uint8_t>& frame, AuthorityWireError& error);
    static bool DecodeCommand(std::span<const uint8_t> frame, AuthorityCommand& command, AuthorityWireError& error);
    static bool EncodeSnapshot(const AuthorityWireSnapshot& snapshot, std::vector<uint8_t>& frame, AuthorityWireError& error);
    static bool DecodeSnapshot(std::span<const uint8_t> frame, AuthorityWireSnapshot& snapshot, AuthorityWireError& error);
};

} // namespace NeoEngine
