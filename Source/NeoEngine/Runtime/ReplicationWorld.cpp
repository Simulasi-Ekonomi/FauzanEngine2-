#include "ReplicationWorld.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace NeoEngine {
namespace {
constexpr uint32_t kMagic = 0x31535052U; // RPS1
constexpr uint32_t kAcknowledgementMagic = 0x314B4341U; // ACK1
constexpr uint16_t kVersion = 1U;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;

void AppendU16(std::vector<uint8_t>& bytes, uint16_t value) {
    bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
}
void AppendU32(std::vector<uint8_t>& bytes, uint32_t value) {
    for (uint8_t shift = 0U; shift < 32U; shift += 8U) bytes.push_back(static_cast<uint8_t>((value >> shift) & 0xFFU));
}
void AppendU64(std::vector<uint8_t>& bytes, uint64_t value) {
    for (uint8_t shift = 0U; shift < 64U; shift += 8U) bytes.push_back(static_cast<uint8_t>((value >> shift) & 0xFFU));
}
void AppendFloat(std::vector<uint8_t>& bytes, float value) {
    uint32_t raw = 0U;
    std::memcpy(&raw, &value, sizeof(raw));
    AppendU32(bytes, raw);
}
bool ReadU16(std::span<const uint8_t> bytes, size_t& offset, uint16_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 2U) return false;
    value = static_cast<uint16_t>(bytes[offset]) | static_cast<uint16_t>(bytes[offset + 1U]) << 8U;
    offset += 2U;
    return true;
}
bool ReadU32(std::span<const uint8_t> bytes, size_t& offset, uint32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 4U) return false;
    value = 0U;
    for (uint8_t shift = 0U; shift < 32U; shift += 8U) value |= static_cast<uint32_t>(bytes[offset + shift / 8U]) << shift;
    offset += 4U;
    return true;
}
bool ReadU64(std::span<const uint8_t> bytes, size_t& offset, uint64_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 8U) return false;
    value = 0U;
    for (uint8_t shift = 0U; shift < 64U; shift += 8U) value |= static_cast<uint64_t>(bytes[offset + shift / 8U]) << shift;
    offset += 8U;
    return true;
}
bool ReadFloat(std::span<const uint8_t> bytes, size_t& offset, float& value) {
    uint32_t raw = 0U;
    if (!ReadU32(bytes, offset, raw)) return false;
    std::memcpy(&value, &raw, sizeof(value));
    return true;
}
uint64_t Hash(std::span<const uint8_t> bytes) {
    uint64_t hash = kHashOffset;
    for (uint8_t byte : bytes) { hash ^= byte; hash *= kHashPrime; }
    return hash;
}
void AppendTransform(std::vector<uint8_t>& bytes, const Transform3& transform) {
    AppendFloat(bytes, transform.x); AppendFloat(bytes, transform.y); AppendFloat(bytes, transform.z);
    AppendFloat(bytes, transform.rx); AppendFloat(bytes, transform.ry); AppendFloat(bytes, transform.rz);
    AppendFloat(bytes, transform.sx); AppendFloat(bytes, transform.sy); AppendFloat(bytes, transform.sz);
}
void AppendSnapshotContent(std::vector<uint8_t>& bytes, const ReplicationSnapshot& snapshot) {
    AppendU32(bytes, kMagic); AppendU16(bytes, kVersion); AppendU64(bytes, snapshot.sequence); AppendU64(bytes, snapshot.serverTick); AppendU16(bytes, snapshot.count);
    for (uint16_t index = 0U; index < snapshot.count; ++index) {
        const ReplicatedEntityState& state = snapshot.states[index];
        AppendU32(bytes, state.networkId); AppendU32(bytes, state.ownerId); AppendU64(bytes, state.stateRevision); AppendTransform(bytes, state.transform);
    }
}
uint64_t SnapshotChecksum(const ReplicationSnapshot& snapshot) {
    try {
        std::vector<uint8_t> content;
        content.reserve(32U + static_cast<size_t>(snapshot.count) * 56U);
        AppendSnapshotContent(content, snapshot);
        return Hash(content);
    } catch (const std::bad_alloc&) {
        return 0U;
    }
}
bool ValidTransform(const Transform3& transform) {
    return std::isfinite(transform.x) && std::isfinite(transform.y) && std::isfinite(transform.z) && std::isfinite(transform.rx) && std::isfinite(transform.ry) && std::isfinite(transform.rz) && std::isfinite(transform.sx) && std::isfinite(transform.sy) && std::isfinite(transform.sz) && transform.sx > 0.0F && transform.sy > 0.0F && transform.sz > 0.0F;
}
bool SameTransform(const Transform3& left, const Transform3& right) {
    return std::memcmp(&left, &right, sizeof(Transform3)) == 0;
}
Transform3 Lerp(const Transform3& from, const Transform3& to, uint16_t alphaPermille) {
    const double alpha = static_cast<double>(alphaPermille) / 1000.0;
    const auto mix = [alpha](float a, float b) { return static_cast<float>(static_cast<double>(a) + (static_cast<double>(b) - static_cast<double>(a)) * alpha); };
    return {mix(from.x, to.x), mix(from.y, to.y), mix(from.z, to.z), mix(from.rx, to.rx), mix(from.ry, to.ry), mix(from.rz, to.rz), mix(from.sx, to.sx), mix(from.sy, to.sy), mix(from.sz, to.sz)};
}
}

bool ReplicationSnapshotCodec::Serialize(const ReplicationSnapshot& snapshot, std::vector<uint8_t>& bytes, ReplicationError& error) {
    if (snapshot.sequence == 0U || snapshot.count > ReplicationSnapshot::kMaxEntities || snapshot.count > 1024U) { error = ReplicationError::InvalidSnapshot; return false; }
    for (uint16_t index = 0U; index < snapshot.count; ++index) {
        const ReplicatedEntityState& state = snapshot.states[index];
        if (state.networkId == 0U || !ValidTransform(state.transform) || (index > 0U && snapshot.states[index - 1U].networkId >= state.networkId)) { error = ReplicationError::InvalidSnapshot; return false; }
    }
    try {
        std::vector<uint8_t> content;
        AppendSnapshotContent(content, snapshot);
        if (content.size() + sizeof(uint64_t) > kMaxBytes) { error = ReplicationError::Capacity; return false; }
        const uint64_t checksum = Hash(content);
        AppendU64(content, checksum);
        bytes = std::move(content);
        error = ReplicationError::None;
        return true;
    } catch (const std::bad_alloc&) {
        error = ReplicationError::Capacity;
        return false;
    }
}

bool ReplicationSnapshotCodec::Deserialize(std::span<const uint8_t> bytes, ReplicationSnapshot& snapshot, ReplicationError& error) {
    if (bytes.size() < 4U + 2U + 8U + 8U + 2U + 8U || bytes.size() > kMaxBytes) { error = ReplicationError::CorruptSnapshot; return false; }
    size_t offset = 0U;
    uint32_t magic = 0U; uint16_t version = 0U; uint64_t sequence = 0U; uint64_t serverTick = 0U; uint16_t count = 0U;
    if (!ReadU32(bytes, offset, magic) || !ReadU16(bytes, offset, version) || !ReadU64(bytes, offset, sequence) || !ReadU64(bytes, offset, serverTick) || !ReadU16(bytes, offset, count) || magic != kMagic || version != kVersion || sequence == 0U || count > ReplicationSnapshot::kMaxEntities) { error = ReplicationError::CorruptSnapshot; return false; }
    ReplicationSnapshot candidate{};
    candidate.sequence = sequence; candidate.serverTick = serverTick; candidate.count = count;
    for (uint16_t index = 0U; index < count; ++index) {
        ReplicatedEntityState& state = candidate.states[index];
        if (!ReadU32(bytes, offset, state.networkId) || !ReadU32(bytes, offset, state.ownerId) || !ReadU64(bytes, offset, state.stateRevision) || !ReadFloat(bytes, offset, state.transform.x) || !ReadFloat(bytes, offset, state.transform.y) || !ReadFloat(bytes, offset, state.transform.z) || !ReadFloat(bytes, offset, state.transform.rx) || !ReadFloat(bytes, offset, state.transform.ry) || !ReadFloat(bytes, offset, state.transform.rz) || !ReadFloat(bytes, offset, state.transform.sx) || !ReadFloat(bytes, offset, state.transform.sy) || !ReadFloat(bytes, offset, state.transform.sz) || state.networkId == 0U || !ValidTransform(state.transform)) { error = ReplicationError::CorruptSnapshot; return false; }
        if (index > 0U && candidate.states[index - 1U].networkId >= state.networkId) { error = ReplicationError::CorruptSnapshot; return false; }
    }
    uint64_t expected = 0U;
    if (!ReadU64(bytes, offset, expected) || offset != bytes.size() || Hash(bytes.first(bytes.size() - sizeof(uint64_t))) != expected) { error = ReplicationError::CorruptSnapshot; return false; }
    candidate.checksum = expected;
    snapshot = std::move(candidate);
    error = ReplicationError::None;
    return true;
}

bool ReplicationAcknowledgementCodec::Serialize(const ReplicationAcknowledgement& acknowledgement, std::vector<uint8_t>& bytes, ReplicationError& error) {
    if (acknowledgement.sequence == 0U || acknowledgement.checksum == 0U) { error = ReplicationError::InvalidAcknowledgement; return false; }
    try {
        std::vector<uint8_t> content;
        content.reserve(30U);
        AppendU32(content, kAcknowledgementMagic); AppendU16(content, kVersion); AppendU64(content, acknowledgement.sequence); AppendU64(content, acknowledgement.serverTick); AppendU64(content, acknowledgement.checksum);
        if (content.size() + sizeof(uint64_t) > kMaxBytes) { error = ReplicationError::Capacity; return false; }
        AppendU64(content, Hash(content));
        bytes = std::move(content);
        error = ReplicationError::None;
        return true;
    } catch (const std::bad_alloc&) {
        error = ReplicationError::Capacity;
        return false;
    }
}

bool ReplicationAcknowledgementCodec::Deserialize(std::span<const uint8_t> bytes, ReplicationAcknowledgement& acknowledgement, ReplicationError& error) {
    if (bytes.size() != 38U) { error = ReplicationError::CorruptSnapshot; return false; }
    size_t offset = 0U;
    uint32_t magic = 0U; uint16_t version = 0U; ReplicationAcknowledgement candidate{}; uint64_t expected = 0U;
    if (!ReadU32(bytes, offset, magic) || !ReadU16(bytes, offset, version) || !ReadU64(bytes, offset, candidate.sequence) || !ReadU64(bytes, offset, candidate.serverTick) || !ReadU64(bytes, offset, candidate.checksum) || !ReadU64(bytes, offset, expected) || magic != kAcknowledgementMagic || version != kVersion || candidate.sequence == 0U || candidate.checksum == 0U || Hash(bytes.first(bytes.size() - sizeof(uint64_t))) != expected) { error = ReplicationError::CorruptSnapshot; return false; }
    acknowledgement = candidate;
    error = ReplicationError::None;
    return true;
}

ReplicationWorld::ReplicationWorld(SceneWorld& sceneWorld, ReplicationRole role, uint32_t localClientId, bool allowDynamicLifecycle) : sceneWorld_(sceneWorld), role_(role), localClientId_(localClientId), allowDynamicLifecycle_(allowDynamicLifecycle && role == ReplicationRole::Client) {}

bool ReplicationWorld::Fail(ReplicationError error) const { lastError_ = error; return false; }
bool ReplicationWorld::ValidTransform(const Transform3& transform) const { return std::isfinite(transform.x) && std::isfinite(transform.y) && std::isfinite(transform.z) && std::isfinite(transform.rx) && std::isfinite(transform.ry) && std::isfinite(transform.rz) && std::isfinite(transform.sx) && std::isfinite(transform.sy) && std::isfinite(transform.sz) && transform.sx > 0.0F && transform.sy > 0.0F && transform.sz > 0.0F; }
ReplicationWorld::Slot* ReplicationWorld::FindSlot(uint32_t networkId) { for (Slot& slot : slots_) if (slot.registered && slot.networkId == networkId) return &slot; return nullptr; }
const ReplicationWorld::Slot* ReplicationWorld::FindSlot(uint32_t networkId) const { for (const Slot& slot : slots_) if (slot.registered && slot.networkId == networkId) return &slot; return nullptr; }
ReplicationWorld::Slot* ReplicationWorld::FindSlot(SceneEntity entity) { for (Slot& slot : slots_) if (slot.registered && slot.entity == entity) return &slot; return nullptr; }

bool ReplicationWorld::RegisterEntity(SceneEntity entity, uint32_t networkId, uint32_t ownerId) {
    if (role_ != ReplicationRole::Server && role_ != ReplicationRole::Client) return Fail(ReplicationError::InvalidInput);
    if (entity.index >= SceneWorld::kCapacity || sceneWorld_.GetTransform(entity) == nullptr) return Fail(ReplicationError::InvalidEntity);
    if (networkId == 0U) return Fail(ReplicationError::InvalidNetworkId);
    if (FindSlot(networkId) != nullptr) return Fail(ReplicationError::DuplicateNetworkId);
    if (FindSlot(entity) != nullptr) return Fail(ReplicationError::DuplicateEntity);
    for (const Slot& slot : slots_) if (slot.registered && slot.entity.index == entity.index && slot.entity != entity) return Fail(ReplicationError::InvalidEntity);
    if (registeredCount_ >= kMaxEntities) return Fail(ReplicationError::Capacity);
    for (Slot& slot : slots_) {
        if (slot.registered) continue;
        const Transform3* transform = sceneWorld_.GetTransform(entity);
        if (transform == nullptr) return Fail(ReplicationError::InvalidEntity);
        slot = {};
        slot.registered = true; slot.entity = entity; slot.networkId = networkId; slot.ownerId = ownerId; slot.authoritative = *transform; slot.previousAuthoritative = *transform; slot.hasAuthoritative = true;
        ++registeredCount_; lastError_ = ReplicationError::None; return true;
    }
    return Fail(ReplicationError::Capacity);
}

bool ReplicationWorld::UnregisterEntity(uint32_t networkId) {
    if (role_ != ReplicationRole::Server && role_ != ReplicationRole::Client) return Fail(ReplicationError::InvalidInput);
    if (networkId == 0U) return Fail(ReplicationError::InvalidNetworkId);
    Slot* slot = FindSlot(networkId);
    if (slot == nullptr) return Fail(ReplicationError::UnknownEntity);
    *slot = {};
    --registeredCount_;
    lastError_ = ReplicationError::None;
    return true;
}

bool ReplicationWorld::BuildServerSnapshot(uint64_t serverTick, ReplicationSnapshot& snapshot) {
    if (role_ != ReplicationRole::Server) return Fail(ReplicationError::NotServer);
    if (serverTick < lastServerTick_) return Fail(ReplicationError::StaleSnapshot);
    if (snapshotSequence_ == std::numeric_limits<uint64_t>::max()) return Fail(ReplicationError::Capacity);
    struct CandidateState { uint16_t slotIndex = 0U; ReplicatedEntityState state{}; Transform3 previous{}; };
    std::array<CandidateState, kMaxEntities> candidates{};
    uint16_t count = 0U;
    for (uint16_t slotIndex = 0U; slotIndex < kMaxEntities; ++slotIndex) {
        Slot& slot = slots_[slotIndex];
        if (!slot.registered) continue;
        const Transform3* transform = sceneWorld_.GetTransform(slot.entity);
        if (transform == nullptr || !ValidTransform(*transform)) return Fail(ReplicationError::InvalidEntity);
        CandidateState& candidate = candidates[count++];
        candidate.slotIndex = slotIndex;
        candidate.previous = slot.authoritative;
        candidate.state = {slot.networkId, slot.ownerId, slot.stateRevision, *transform};
        if (!slot.hasAuthoritative || !SameTransform(slot.authoritative, *transform)) {
            if (slot.stateRevision == std::numeric_limits<uint64_t>::max()) return Fail(ReplicationError::Capacity);
            candidate.state.stateRevision = slot.stateRevision == 0U ? 1U : slot.stateRevision + 1U;
        }
    }
    std::sort(candidates.begin(), candidates.begin() + count, [](const CandidateState& left, const CandidateState& right) { return left.state.networkId < right.state.networkId; });
    ReplicationSnapshot snapshotCandidate{};
    snapshotCandidate.sequence = snapshotSequence_ + 1U;
    snapshotCandidate.serverTick = serverTick;
    snapshotCandidate.count = count;
    for (uint16_t index = 0U; index < count; ++index) snapshotCandidate.states[index] = candidates[index].state;
    snapshotCandidate.checksum = SnapshotChecksum(snapshotCandidate);
    if (snapshotCandidate.checksum == 0U) return Fail(ReplicationError::Capacity);
    for (uint16_t index = 0U; index < count; ++index) {
        Slot& slot = slots_[candidates[index].slotIndex];
        slot.previousAuthoritative = candidates[index].previous;
        slot.authoritative = candidates[index].state.transform;
        slot.stateRevision = candidates[index].state.stateRevision;
        slot.hasAuthoritative = true;
    }
    snapshot = std::move(snapshotCandidate);
    snapshotSequence_ = snapshot.sequence;
    lastServerTick_ = serverTick;
    lastSnapshotChecksum_ = snapshot.checksum;
    acknowledgementHistory_[snapshotSequence_ % kMaxAcknowledgementHistory] = {snapshotSequence_, lastServerTick_, lastSnapshotChecksum_};
    lastError_ = ReplicationError::None;
    return true;
}

bool ReplicationWorld::ValidateSnapshot(const ReplicationSnapshot& snapshot) const {
    if (snapshot.sequence == 0U || snapshot.count > kMaxEntities || snapshot.checksum == 0U || snapshot.checksum != SnapshotChecksum(snapshot)) return false;
    for (uint16_t index = 0U; index < snapshot.count; ++index) {
        const ReplicatedEntityState& state = snapshot.states[index];
        if (state.networkId == 0U || !ValidTransform(state.transform) || (index > 0U && snapshot.states[index - 1U].networkId >= state.networkId)) return false;
    }
    return true;
}

bool ReplicationWorld::ApplyTransform(const Slot& slot, const Transform3& transform) {
    return sceneWorld_.SetTransform(slot.entity, transform);
}

bool ReplicationWorld::SetDynamicLifecycleEnabled(bool enabled) {
    if (role_ != ReplicationRole::Client) return Fail(ReplicationError::NotClient);
    allowDynamicLifecycle_ = enabled;
    lastError_ = ReplicationError::None;
    return true;
}

bool ReplicationWorld::BuildClientAcknowledgement(ReplicationAcknowledgement& acknowledgement) const {
    if (role_ != ReplicationRole::Client || snapshotSequence_ == 0U || lastSnapshotChecksum_ == 0U) return Fail(role_ == ReplicationRole::Client ? ReplicationError::InvalidAcknowledgement : ReplicationError::NotClient);
    acknowledgement = {snapshotSequence_, lastServerTick_, lastSnapshotChecksum_};
    lastError_ = ReplicationError::None;
    return true;
}

bool ReplicationWorld::ApplyClientAcknowledgement(const ReplicationAcknowledgement& acknowledgement) {
    if (role_ != ReplicationRole::Server) return Fail(ReplicationError::NotServer);
    if (acknowledgement.sequence == 0U || acknowledgement.sequence > snapshotSequence_ || acknowledgement.checksum == 0U) return Fail(ReplicationError::InvalidAcknowledgement);
    const AcknowledgementRecord& record = acknowledgementHistory_[acknowledgement.sequence % kMaxAcknowledgementHistory];
    if (record.sequence != acknowledgement.sequence) return Fail(ReplicationError::StaleAcknowledgement);
    if (acknowledgement.serverTick != record.serverTick || acknowledgement.checksum != record.checksum) return Fail(ReplicationError::InvalidAcknowledgement);
    if (acknowledgement.sequence < acknowledgedSequence_) return Fail(ReplicationError::StaleAcknowledgement);
    acknowledgedSequence_ = acknowledgement.sequence;
    acknowledgedServerTick_ = acknowledgement.serverTick;
    lastError_ = ReplicationError::None;
    return true;
}

bool ReplicationWorld::ApplyServerSnapshot(const ReplicationSnapshot& snapshot, ReplicationApplyReceipt& receipt) {
    if (role_ != ReplicationRole::Client) return Fail(ReplicationError::NotClient);
    if (!ValidateSnapshot(snapshot)) return Fail(ReplicationError::InvalidSnapshot);
    if (snapshot.sequence <= snapshotSequence_ || snapshot.serverTick < lastServerTick_) return Fail(ReplicationError::StaleSnapshot);
    std::array<uint16_t, kMaxEntities> resolvedSlots{};
    std::array<SceneEntity, kMaxEntities> spawnedEntities{};
    std::array<bool, kMaxEntities> usedSlots{};
    std::array<bool, kMaxEntities> presentSlots{};
    resolvedSlots.fill(0xFFFFU);
    for (uint16_t index = 0U; index < kMaxEntities; ++index) usedSlots[index] = slots_[index].registered;
    uint16_t spawnCount = 0U;
    for (uint16_t index = 0U; index < snapshot.count; ++index) {
        const ReplicatedEntityState& state = snapshot.states[index];
        const Slot* slot = FindSlot(state.networkId);
        if (slot == nullptr) {
            if (!allowDynamicLifecycle_) return Fail(ReplicationError::UnknownEntity);
            if (registeredCount_ + spawnCount >= kMaxEntities) return Fail(ReplicationError::Capacity);
            uint16_t freeSlot = 0xFFFFU;
            for (uint16_t candidate = 0U; candidate < kMaxEntities; ++candidate) if (!usedSlots[candidate]) { freeSlot = candidate; break; }
            if (freeSlot == 0xFFFFU) return Fail(ReplicationError::SpawnRejected);
            usedSlots[freeSlot] = true;
            resolvedSlots[index] = freeSlot;
            ++spawnCount;
        } else {
            const uint16_t slotIndex = static_cast<uint16_t>(slot - slots_.data());
            resolvedSlots[index] = slotIndex;
            presentSlots[slotIndex] = true;
            if (state.stateRevision < slot->stateRevision) return Fail(ReplicationError::StaleSnapshot);
            if (sceneWorld_.GetTransform(slot->entity) == nullptr) return Fail(ReplicationError::InvalidEntity);
        }
    }
    std::unique_ptr<SceneWorld> candidateScene;
    try { candidateScene = std::make_unique<SceneWorld>(sceneWorld_); }
    catch (const std::bad_alloc&) { return Fail(ReplicationError::Capacity); }
    ReplicationApplyReceipt candidateReceipt{};
    candidateReceipt.sequence = snapshot.sequence;
    candidateReceipt.serverTick = snapshot.serverTick;
    uint16_t spawnedIndex = 0U;
    for (uint16_t index = 0U; index < snapshot.count; ++index) {
        const ReplicatedEntityState& state = snapshot.states[index];
        const uint16_t slotIndex = resolvedSlots[index];
        Slot* slot = FindSlot(state.networkId);
        if (slot == nullptr) {
            SceneEntity entity{};
            if (!candidateScene->Create(entity) || !candidateScene->SetTransform(entity, state.transform)) return Fail(ReplicationError::SpawnRejected);
            spawnedEntities[spawnedIndex++] = entity;
            if (slotIndex >= kMaxEntities) return Fail(ReplicationError::SpawnRejected);
            presentSlots[slotIndex] = true;
            ++candidateReceipt.spawnedEntities;
        } else if (state.ownerId == localClientId_) {
            if (slot->hasPrediction && !SameTransform(slot->predictedTransform, state.transform)) ++candidateReceipt.reconciledPredictions;
            if (!candidateScene->SetTransform(slot->entity, state.transform)) return Fail(ReplicationError::SceneApplyRejected);
        }
        ++candidateReceipt.appliedEntities;
    }
    if (allowDynamicLifecycle_) for (uint16_t slotIndex = 0U; slotIndex < kMaxEntities; ++slotIndex) if (slots_[slotIndex].registered && !presentSlots[slotIndex]) {
        if (!candidateScene->Destroy(slots_[slotIndex].entity)) return Fail(ReplicationError::DespawnRejected);
        ++candidateReceipt.despawnedEntities;
    }
    sceneWorld_ = *candidateScene;
    uint16_t spawnedCommitIndex = 0U;
    for (uint16_t index = 0U; index < snapshot.count; ++index) {
        const ReplicatedEntityState& state = snapshot.states[index];
        const uint16_t slotIndex = resolvedSlots[index];
        Slot& slot = slots_[slotIndex];
        if (!slot.registered) {
            slot = {};
            slot.registered = true;
            slot.entity = spawnedEntities[spawnedCommitIndex++];
            slot.networkId = state.networkId;
            slot.ownerId = state.ownerId;
            slot.authoritative = state.transform;
            slot.previousAuthoritative = state.transform;
            slot.stateRevision = state.stateRevision;
            slot.hasAuthoritative = true;
            ++registeredCount_;
            continue;
        }
        slot.hasPrediction = false;
        slot.previousAuthoritative = slot.authoritative;
        slot.authoritative = state.transform;
        slot.ownerId = state.ownerId;
        slot.stateRevision = state.stateRevision;
        slot.hasAuthoritative = true;
    }
    if (allowDynamicLifecycle_) for (uint16_t slotIndex = 0U; slotIndex < kMaxEntities; ++slotIndex) if (slots_[slotIndex].registered && !presentSlots[slotIndex]) {
        slots_[slotIndex] = {};
        --registeredCount_;
    }
    snapshotSequence_ = snapshot.sequence;
    lastServerTick_ = snapshot.serverTick;
    lastSnapshotChecksum_ = snapshot.checksum;
    candidateReceipt.accepted = true;
    receipt = candidateReceipt;
    lastError_ = ReplicationError::None;
    return true;
}

bool ReplicationWorld::SetInterpolationAlphaPermille(uint16_t alphaPermille) {
    if (role_ != ReplicationRole::Client) return Fail(ReplicationError::NotClient);
    if (alphaPermille > kMaxInterpolationPermille) return Fail(ReplicationError::InvalidInput);
    interpolationAlphaPermille_ = alphaPermille; lastError_ = ReplicationError::None; return true;
}

bool ReplicationWorld::ApplyInterpolation(ReplicationApplyReceipt& receipt) {
    if (role_ != ReplicationRole::Client) return Fail(ReplicationError::NotClient);
    std::unique_ptr<SceneWorld> candidateScene;
    try { candidateScene = std::make_unique<SceneWorld>(sceneWorld_); }
    catch (const std::bad_alloc&) { return Fail(ReplicationError::Capacity); }
    uint16_t interpolated = 0U;
    for (const Slot& slot : slots_) {
        if (!slot.registered || slot.ownerId == localClientId_ || !slot.hasAuthoritative) continue;
        const Transform3 candidate = Lerp(slot.previousAuthoritative, slot.authoritative, interpolationAlphaPermille_);
        if (!candidateScene->SetTransform(slot.entity, candidate)) return Fail(ReplicationError::SceneApplyRejected);
        ++interpolated;
    }
    sceneWorld_ = *candidateScene;
    ReplicationApplyReceipt candidateReceipt{};
    candidateReceipt.sequence = snapshotSequence_;
    candidateReceipt.serverTick = lastServerTick_;
    candidateReceipt.interpolatedEntities = interpolated;
    candidateReceipt.accepted = true;
    receipt = candidateReceipt;
    lastError_ = ReplicationError::None;
    return true;
}

bool ReplicationWorld::PredictLocalInput(uint32_t networkId, float deltaX, float deltaZ, ReplicationPredictionReceipt& receipt) {
    if (role_ != ReplicationRole::Client) return Fail(ReplicationError::NotClient);
    if (!std::isfinite(deltaX) || !std::isfinite(deltaZ) || std::abs(deltaX) > kMaxPredictionDelta || std::abs(deltaZ) > kMaxPredictionDelta) return Fail(ReplicationError::InvalidInput);
    Slot* slot = FindSlot(networkId);
    if (slot == nullptr) return Fail(ReplicationError::UnknownEntity);
    if (slot->ownerId != localClientId_) return Fail(ReplicationError::OwnershipRejected);
    const Transform3* current = sceneWorld_.GetTransform(slot->entity);
    if (current == nullptr) return Fail(ReplicationError::InvalidEntity);
    Transform3 candidate = *current; candidate.x += deltaX; candidate.z += deltaZ;
    if (!ValidTransform(candidate) || predictionSequence_ == std::numeric_limits<uint64_t>::max()) return Fail(ReplicationError::InvalidInput);
    if (!sceneWorld_.SetTransform(slot->entity, candidate)) return Fail(ReplicationError::SceneApplyRejected);
    slot->predictedTransform = candidate; slot->hasPrediction = true; ++predictionSequence_;
    receipt = {networkId, predictionSequence_, candidate}; lastError_ = ReplicationError::None; return true;
}

bool ReplicationWorld::IsRegistered(uint32_t networkId) const { return FindSlot(networkId) != nullptr; }

bool ReplicationWorld::AuthoritativeState(uint32_t networkId, ReplicatedEntityState& state) const {
    const Slot* slot = FindSlot(networkId);
    if (slot == nullptr || !slot->hasAuthoritative) return Fail(ReplicationError::UnknownEntity);
    state = {slot->networkId, slot->ownerId, slot->stateRevision, slot->authoritative};
    lastError_ = ReplicationError::None;
    return true;
}

} // namespace NeoEngine
