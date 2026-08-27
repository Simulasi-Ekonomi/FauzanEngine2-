#pragma once

#include "FarmWorldTool.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {

enum class FarmCanonicalToolError : uint8_t {
    None,
    NotInitialized,
    InvalidArgument,
    InvalidWorld,
    InvalidRules,
    InvalidContent,
    ContentCapacity,
    InvalidSequence,
    InvalidCommand,
    UnsupportedVersion,
    MalformedPayload,
    ChecksumMismatch,
    MigrationFailed,
    WorldLoadFailed,
};

enum class FarmToolCommandKind : uint8_t { Move, Till };

struct FarmToolRules {
    uint16_t maxCommands = 256;
    uint16_t maxContentEntries = 128;
    uint16_t maxReplaySteps = 256;
    uint16_t maxMoveDistance = 1;
};

struct FarmToolContentEntry {
    std::string id;
    uint32_t revision = 1;
    uint64_t assetHash = 0;
};

struct FarmToolCommand {
    uint64_t sequence = 0;
    FarmToolCommandKind kind = FarmToolCommandKind::Move;
    int16_t dx = 0;
    int16_t dz = 0;
    uint16_t x = 0;
    uint16_t z = 0;
};

struct FarmToolCommandReceipt {
    bool accepted = false;
    FarmCanonicalToolError error = FarmCanonicalToolError::None;
    uint64_t sequence = 0;
    uint64_t deterministicState = 0;
};

struct FarmToolReplayReceipt {
    bool accepted = false;
    FarmCanonicalToolError error = FarmCanonicalToolError::None;
    uint16_t appliedCommands = 0;
    uint64_t deterministicState = 0;
};

class FarmCanonicalGameTool {
public:
    static constexpr uint16_t kCurrentVersion = 2;
    static constexpr uint16_t kLegacyVersion = 1;

    bool Initialize(FarmWorldTool& world, FarmToolRules rules = {});
    bool RegisterContent(std::string id, uint32_t revision, uint64_t assetHash);
    bool ApplyRules(FarmToolRules rules);
    FarmToolCommandReceipt Apply(const FarmToolCommand& command);
    FarmToolReplayReceipt Replay(std::span<const FarmToolCommand> commands);

    [[nodiscard]] std::vector<uint8_t> Save() const;
    bool Load(std::span<const uint8_t> bytes);
    bool MigrateToCurrent(std::span<const uint8_t> legacyBytes, std::vector<uint8_t>& migrated) const;
    [[nodiscard]] std::vector<uint8_t> SerializeLegacyV1ForCompatibility() const;

    [[nodiscard]] bool HasContent(std::string_view id) const;
    [[nodiscard]] const FarmToolRules& Rules() const { return rules_; }
    [[nodiscard]] std::span<const FarmToolContentEntry> Content() const { return content_; }
    [[nodiscard]] uint64_t DeterministicState() const;
    [[nodiscard]] FarmCanonicalToolError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return world_ != nullptr; }
    [[nodiscard]] uint64_t LastAppliedSequence() const { return lastAppliedSequence_; }

private:
    bool Fail(FarmCanonicalToolError error);
    bool ValidateRules(FarmToolRules rules) const;
    bool ValidateContentEntry(const FarmToolContentEntry& entry) const;
    bool ValidateCommand(const FarmToolCommand& command, uint64_t previousSequence) const;
    bool ParseCurrent(std::span<const uint8_t> bytes);
    bool ApplyMove(const FarmToolCommand& command);
    bool ApplyTill(const FarmToolCommand& command);

    FarmWorldTool* world_ = nullptr;
    FarmToolRules rules_{};
    std::vector<FarmToolContentEntry> content_;
    uint64_t lastAppliedSequence_ = 0;
    FarmCanonicalToolError lastError_ = FarmCanonicalToolError::None;
};

} // namespace NeoEngine
