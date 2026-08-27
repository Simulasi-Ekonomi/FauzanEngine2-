#include "FarmCanonicalGameTool.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <limits>

namespace NeoEngine {
namespace {

constexpr uint32_t kMagic = 0x4C544F4FU; // "OTOL" in little-endian bytes: tool-owned payload.
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;
constexpr size_t kMaxPayloadBytes = 16U * 1024U * 1024U;

uint64_t HashBytes(std::span<const uint8_t> bytes) {
    uint64_t hash = kHashOffset;
    for (const uint8_t byte : bytes) {
        hash ^= byte;
        hash *= kHashPrime;
    }
    return hash;
}

template <typename T>
void Append(std::vector<uint8_t>& output, T value) {
    for (size_t byte = 0; byte < sizeof(T); ++byte) {
        output.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (byte * 8U)) & 0xFFU));
    }
}

template <typename T>
bool Read(std::span<const uint8_t> input, size_t& offset, T& value) {
    if (offset > input.size() || sizeof(T) > input.size() - offset) return false;
    uint64_t raw = 0;
    for (size_t byte = 0; byte < sizeof(T); ++byte) {
        raw |= static_cast<uint64_t>(input[offset + byte]) << (byte * 8U);
    }
    value = static_cast<T>(raw);
    offset += sizeof(T);
    return true;
}

void AppendString(std::vector<uint8_t>& output, std::string_view value) {
    Append<uint16_t>(output, static_cast<uint16_t>(value.size()));
    output.insert(output.end(), value.begin(), value.end());
}

bool ReadString(std::span<const uint8_t> input, size_t& offset, std::string& value) {
    uint16_t length = 0;
    if (!Read(input, offset, length) || offset > input.size() || length > input.size() - offset) return false;
    value.assign(reinterpret_cast<const char*>(input.data() + offset), length);
    offset += length;
    return true;
}

void AppendChecksum(std::vector<uint8_t>& output) {
    Append<uint64_t>(output, HashBytes(output));
}

bool HasValidChecksum(std::span<const uint8_t> bytes) {
    if (bytes.size() < sizeof(uint64_t)) return false;
    size_t offset = bytes.size() - sizeof(uint64_t);
    uint64_t expected = 0;
    if (!Read(bytes, offset, expected)) return false;
    return HashBytes(bytes.first(bytes.size() - sizeof(uint64_t))) == expected;
}

} // namespace

bool FarmCanonicalGameTool::Initialize(FarmWorldTool& world, FarmToolRules rules) {
    if (!world.IsReady() || !ValidateRules(rules)) return Fail(FarmCanonicalToolError::InvalidWorld);
    world_ = &world;
    rules_ = rules;
    content_.clear();
    content_.reserve(rules_.maxContentEntries);
    lastAppliedSequence_ = 0;
    lastError_ = FarmCanonicalToolError::None;
    return true;
}

bool FarmCanonicalGameTool::RegisterContent(std::string id, uint32_t revision, uint64_t assetHash) {
    if (!IsReady()) return Fail(FarmCanonicalToolError::NotInitialized);
    FarmToolContentEntry entry{std::move(id), revision, assetHash};
    if (!ValidateContentEntry(entry)) return Fail(FarmCanonicalToolError::InvalidContent);
    const auto position = std::lower_bound(content_.begin(), content_.end(), entry.id,
        [](const FarmToolContentEntry& value, std::string_view key) { return value.id < key; });
    if (position != content_.end() && position->id == entry.id) return Fail(FarmCanonicalToolError::InvalidContent);
    if (content_.size() >= rules_.maxContentEntries) return Fail(FarmCanonicalToolError::ContentCapacity);
    content_.insert(position, std::move(entry));
    lastError_ = FarmCanonicalToolError::None;
    return true;
}

bool FarmCanonicalGameTool::ApplyRules(FarmToolRules rules) {
    if (!IsReady()) return Fail(FarmCanonicalToolError::NotInitialized);
    if (!ValidateRules(rules)) return Fail(FarmCanonicalToolError::InvalidRules);
    if (content_.size() > rules.maxContentEntries) return Fail(FarmCanonicalToolError::InvalidRules);
    rules_ = rules;
    lastError_ = FarmCanonicalToolError::None;
    return true;
}

FarmToolCommandReceipt FarmCanonicalGameTool::Apply(const FarmToolCommand& command) {
    FarmToolCommandReceipt receipt{false, FarmCanonicalToolError::None, command.sequence, DeterministicState()};
    if (!IsReady()) {
        receipt.error = FarmCanonicalToolError::NotInitialized;
        lastError_ = receipt.error;
        return receipt;
    }
    if (!ValidateCommand(command, lastAppliedSequence_)) {
        receipt.error = command.sequence <= lastAppliedSequence_ || command.sequence == 0 ? FarmCanonicalToolError::InvalidSequence : FarmCanonicalToolError::InvalidCommand;
        lastError_ = receipt.error;
        return receipt;
    }
    const bool applied = command.kind == FarmToolCommandKind::Move ? ApplyMove(command) : ApplyTill(command);
    if (!applied) {
        receipt.error = FarmCanonicalToolError::InvalidCommand;
        lastError_ = receipt.error;
        receipt.deterministicState = DeterministicState();
        return receipt;
    }
    lastAppliedSequence_ = command.sequence;
    lastError_ = FarmCanonicalToolError::None;
    receipt.accepted = true;
    receipt.deterministicState = DeterministicState();
    return receipt;
}

FarmToolReplayReceipt FarmCanonicalGameTool::Replay(std::span<const FarmToolCommand> commands) {
    FarmToolReplayReceipt receipt{false, FarmCanonicalToolError::None, 0, DeterministicState()};
    if (!IsReady()) {
        receipt.error = FarmCanonicalToolError::NotInitialized;
        lastError_ = receipt.error;
        return receipt;
    }
    if (commands.size() > rules_.maxReplaySteps) {
        receipt.error = FarmCanonicalToolError::InvalidCommand;
        lastError_ = receipt.error;
        return receipt;
    }
    uint64_t sequence = lastAppliedSequence_;
    for (const FarmToolCommand& command : commands) {
        if (!ValidateCommand(command, sequence)) {
            receipt.error = command.sequence <= sequence || command.sequence == 0 ? FarmCanonicalToolError::InvalidSequence : FarmCanonicalToolError::InvalidCommand;
            lastError_ = receipt.error;
            return receipt;
        }
        sequence = command.sequence;
    }
    const std::vector<uint8_t> checkpoint = Save();
    if (checkpoint.empty()) {
        receipt.error = FarmCanonicalToolError::MalformedPayload;
        lastError_ = receipt.error;
        return receipt;
    }
    for (const FarmToolCommand& command : commands) {
        const FarmToolCommandReceipt commandReceipt = Apply(command);
        if (!commandReceipt.accepted) {
            const FarmCanonicalToolError failure = commandReceipt.error;
            if (!Load(checkpoint)) return {false, FarmCanonicalToolError::WorldLoadFailed, receipt.appliedCommands, DeterministicState()};
            lastError_ = failure;
            return {false, failure, receipt.appliedCommands, DeterministicState()};
        }
        ++receipt.appliedCommands;
    }
    lastError_ = FarmCanonicalToolError::None;
    receipt.accepted = true;
    receipt.deterministicState = DeterministicState();
    return receipt;
}

std::vector<uint8_t> FarmCanonicalGameTool::Save() const {
    if (!IsReady()) return {};
    std::vector<uint8_t> output;
    output.reserve(64U + content_.size() * 32U);
    Append<uint32_t>(output, kMagic);
    Append<uint16_t>(output, kCurrentVersion);
    Append<uint16_t>(output, rules_.maxCommands);
    Append<uint16_t>(output, rules_.maxContentEntries);
    Append<uint16_t>(output, rules_.maxReplaySteps);
    Append<uint16_t>(output, rules_.maxMoveDistance);
    Append<uint32_t>(output, static_cast<uint32_t>(content_.size()));
    for (const FarmToolContentEntry& entry : content_) {
        AppendString(output, entry.id);
        Append<uint32_t>(output, entry.revision);
        Append<uint64_t>(output, entry.assetHash);
    }
    const std::vector<uint8_t> worldBytes = world_->Serialize();
    if (worldBytes.empty() || worldBytes.size() > kMaxPayloadBytes) return {};
    Append<uint32_t>(output, static_cast<uint32_t>(worldBytes.size()));
    output.insert(output.end(), worldBytes.begin(), worldBytes.end());
    Append<uint64_t>(output, lastAppliedSequence_);
    AppendChecksum(output);
    return output;
}

bool FarmCanonicalGameTool::Load(std::span<const uint8_t> bytes) {
    if (!IsReady()) return Fail(FarmCanonicalToolError::NotInitialized);
    if (bytes.size() < sizeof(uint32_t) + sizeof(uint16_t)) return Fail(FarmCanonicalToolError::MalformedPayload);
    size_t versionOffset = sizeof(uint32_t);
    uint16_t version = 0;
    if (!Read(bytes, versionOffset, version)) return Fail(FarmCanonicalToolError::MalformedPayload);
    if (version == kLegacyVersion) {
        std::vector<uint8_t> migrated;
        if (!MigrateToCurrent(bytes, migrated)) return false;
        return ParseCurrent(migrated);
    }
    return ParseCurrent(bytes);
}

bool FarmCanonicalGameTool::MigrateToCurrent(std::span<const uint8_t> legacyBytes, std::vector<uint8_t>& migrated) const {
    if (!IsReady() || legacyBytes.size() > kMaxPayloadBytes || !HasValidChecksum(legacyBytes)) return false;
    size_t offset = 0;
    uint32_t magic = 0;
    uint16_t version = 0;
    FarmToolRules rules{};
    uint32_t contentCount = 0;
    uint32_t worldByteCount = 0;
    uint64_t lastSequence = 0;
    if (!Read(legacyBytes, offset, magic) || !Read(legacyBytes, offset, version) || magic != kMagic || version != kLegacyVersion ||
        !Read(legacyBytes, offset, rules.maxCommands) || !Read(legacyBytes, offset, rules.maxContentEntries) || !Read(legacyBytes, offset, rules.maxReplaySteps) ||
        !Read(legacyBytes, offset, contentCount) || contentCount > rules.maxContentEntries || !ValidateRules(rules)) return false;
    std::vector<FarmToolContentEntry> content;
    content.reserve(contentCount);
    for (uint32_t index = 0; index < contentCount; ++index) {
        FarmToolContentEntry entry{};
        if (!ReadString(legacyBytes, offset, entry.id) || !Read(legacyBytes, offset, entry.revision) || !Read(legacyBytes, offset, entry.assetHash) ||
            !ValidateContentEntry(entry) || (!content.empty() && content.back().id >= entry.id)) return false;
        content.push_back(std::move(entry));
    }
    if (!Read(legacyBytes, offset, worldByteCount) || worldByteCount == 0U || offset > legacyBytes.size() || worldByteCount > legacyBytes.size() - offset) return false;
    const size_t worldOffset = offset;
    offset += worldByteCount;
    if (!Read(legacyBytes, offset, lastSequence) || offset + sizeof(uint64_t) != legacyBytes.size()) return false;
    migrated.clear();
    migrated.reserve(legacyBytes.size() + sizeof(uint16_t));
    Append<uint32_t>(migrated, kMagic);
    Append<uint16_t>(migrated, kCurrentVersion);
    Append<uint16_t>(migrated, rules.maxCommands);
    Append<uint16_t>(migrated, rules.maxContentEntries);
    Append<uint16_t>(migrated, rules.maxReplaySteps);
    Append<uint16_t>(migrated, 1U);
    Append<uint32_t>(migrated, contentCount);
    for (const FarmToolContentEntry& entry : content) {
        AppendString(migrated, entry.id);
        Append<uint32_t>(migrated, entry.revision);
        Append<uint64_t>(migrated, entry.assetHash);
    }
    Append<uint32_t>(migrated, worldByteCount);
    migrated.insert(migrated.end(), legacyBytes.begin() + static_cast<std::ptrdiff_t>(worldOffset), legacyBytes.begin() + static_cast<std::ptrdiff_t>(worldOffset + worldByteCount));
    Append<uint64_t>(migrated, lastSequence);
    AppendChecksum(migrated);
    return true;
}

std::vector<uint8_t> FarmCanonicalGameTool::SerializeLegacyV1ForCompatibility() const {
    if (!IsReady()) return {};
    const std::vector<uint8_t> worldBytes = world_->Serialize();
    if (worldBytes.empty() || worldBytes.size() > kMaxPayloadBytes || rules_.maxContentEntries == 0) return {};
    std::vector<uint8_t> output;
    Append<uint32_t>(output, kMagic);
    Append<uint16_t>(output, kLegacyVersion);
    Append<uint16_t>(output, rules_.maxCommands);
    Append<uint16_t>(output, rules_.maxContentEntries);
    Append<uint16_t>(output, rules_.maxReplaySteps);
    Append<uint32_t>(output, static_cast<uint32_t>(content_.size()));
    for (const FarmToolContentEntry& entry : content_) {
        AppendString(output, entry.id);
        Append<uint32_t>(output, entry.revision);
        Append<uint64_t>(output, entry.assetHash);
    }
    Append<uint32_t>(output, static_cast<uint32_t>(worldBytes.size()));
    output.insert(output.end(), worldBytes.begin(), worldBytes.end());
    Append<uint64_t>(output, lastAppliedSequence_);
    AppendChecksum(output);
    return output;
}

bool FarmCanonicalGameTool::HasContent(std::string_view id) const {
    const auto position = std::lower_bound(content_.begin(), content_.end(), id,
        [](const FarmToolContentEntry& value, std::string_view key) { return value.id < key; });
    return position != content_.end() && position->id == id;
}

uint64_t FarmCanonicalGameTool::DeterministicState() const {
    uint64_t hash = kHashOffset;
    const auto mix = [&hash](uint64_t value) {
        for (uint8_t byte = 0; byte < sizeof(value); ++byte) {
            hash ^= static_cast<uint8_t>(value & 0xFFU);
            hash *= kHashPrime;
            value >>= 8U;
        }
    };
    mix(world_ == nullptr ? 0U : world_->DeterministicState());
    mix(rules_.maxCommands); mix(rules_.maxContentEntries); mix(rules_.maxReplaySteps); mix(rules_.maxMoveDistance); mix(lastAppliedSequence_);
    for (const FarmToolContentEntry& entry : content_) {
        for (const unsigned char byte : entry.id) mix(byte);
        mix(entry.revision); mix(entry.assetHash);
    }
    return hash;
}

bool FarmCanonicalGameTool::Fail(FarmCanonicalToolError error) {
    lastError_ = error;
    return false;
}

bool FarmCanonicalGameTool::ValidateRules(FarmToolRules rules) const {
    return rules.maxCommands > 0U && rules.maxCommands <= 4096U && rules.maxContentEntries > 0U && rules.maxContentEntries <= 1024U &&
        rules.maxReplaySteps > 0U && rules.maxReplaySteps <= 4096U && rules.maxMoveDistance > 0U && rules.maxMoveDistance <= 32U;
}

bool FarmCanonicalGameTool::ValidateContentEntry(const FarmToolContentEntry& entry) const {
    if (entry.id.empty() || entry.id.size() > 96U || entry.revision == 0U || entry.assetHash == 0U ||
        entry.id.find("..") != std::string::npos || entry.id.front() == '/' || entry.id.back() == '/' || entry.id.find("//") != std::string::npos) return false;
    for (const unsigned char byte : entry.id) {
        if (!(std::isalnum(byte) != 0 || byte == '.' || byte == '_' || byte == '-' || byte == '/')) return false;
    }
    return true;
}

bool FarmCanonicalGameTool::ValidateCommand(const FarmToolCommand& command, uint64_t previousSequence) const {
    if (command.sequence == 0U || command.sequence <= previousSequence || static_cast<uint8_t>(command.kind) > static_cast<uint8_t>(FarmToolCommandKind::Till)) return false;
    if (command.kind == FarmToolCommandKind::Move &&
        (command.dx > static_cast<int16_t>(rules_.maxMoveDistance) || command.dx < -static_cast<int16_t>(rules_.maxMoveDistance) ||
         command.dz > static_cast<int16_t>(rules_.maxMoveDistance) || command.dz < -static_cast<int16_t>(rules_.maxMoveDistance))) return false;
    return true;
}

bool FarmCanonicalGameTool::ParseCurrent(std::span<const uint8_t> bytes) {
    if (!IsReady() || bytes.size() > kMaxPayloadBytes || !HasValidChecksum(bytes)) return Fail(FarmCanonicalToolError::MalformedPayload);
    size_t offset = 0;
    uint32_t magic = 0;
    uint16_t version = 0;
    FarmToolRules rules{};
    uint32_t contentCount = 0;
    uint32_t worldByteCount = 0;
    uint64_t lastSequence = 0;
    if (!Read(bytes, offset, magic) || !Read(bytes, offset, version) || magic != kMagic || version != kCurrentVersion ||
        !Read(bytes, offset, rules.maxCommands) || !Read(bytes, offset, rules.maxContentEntries) || !Read(bytes, offset, rules.maxReplaySteps) ||
        !Read(bytes, offset, rules.maxMoveDistance) || !ValidateRules(rules) || !Read(bytes, offset, contentCount) || contentCount > rules.maxContentEntries) {
        return Fail(version != kCurrentVersion ? FarmCanonicalToolError::UnsupportedVersion : FarmCanonicalToolError::MalformedPayload);
    }
    std::vector<FarmToolContentEntry> content;
    content.reserve(contentCount);
    for (uint32_t index = 0; index < contentCount; ++index) {
        FarmToolContentEntry entry{};
        if (!ReadString(bytes, offset, entry.id) || !Read(bytes, offset, entry.revision) || !Read(bytes, offset, entry.assetHash) ||
            !ValidateContentEntry(entry) || (!content.empty() && content.back().id >= entry.id)) return Fail(FarmCanonicalToolError::InvalidContent);
        content.push_back(std::move(entry));
    }
    if (!Read(bytes, offset, worldByteCount) || worldByteCount == 0U || worldByteCount > kMaxPayloadBytes || offset > bytes.size() ||
        worldByteCount > bytes.size() - offset) return Fail(FarmCanonicalToolError::MalformedPayload);
    const size_t worldOffset = offset;
    offset += worldByteCount;
    if (!Read(bytes, offset, lastSequence) || offset + sizeof(uint64_t) != bytes.size()) return Fail(FarmCanonicalToolError::MalformedPayload);
    const std::span<const uint8_t> worldBytes = bytes.subspan(worldOffset, worldByteCount);
    if (!world_->Deserialize(worldBytes)) return Fail(FarmCanonicalToolError::WorldLoadFailed);
    rules_ = rules;
    content_ = std::move(content);
    lastAppliedSequence_ = lastSequence;
    lastError_ = FarmCanonicalToolError::None;
    return true;
}

bool FarmCanonicalGameTool::ApplyMove(const FarmToolCommand& command) {
    const FarmCharacterState current = world_->Character();
    const int nextX = static_cast<int>(current.x) + command.dx;
    const int nextZ = static_cast<int>(current.z) + command.dz;
    if (nextX < 0 || nextZ < 0 || nextX > std::numeric_limits<uint16_t>::max() || nextZ > std::numeric_limits<uint16_t>::max()) return false;
    return world_->SetCharacterState({static_cast<uint16_t>(nextX), static_cast<uint16_t>(nextZ), current.level});
}

bool FarmCanonicalGameTool::ApplyTill(const FarmToolCommand& command) {
    return world_->PlayerTill(command.x, command.z);
}

} // namespace NeoEngine
