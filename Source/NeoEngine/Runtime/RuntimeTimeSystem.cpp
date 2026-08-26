#include "RuntimeTimeSystem.h"

#include <algorithm>
#include <array>
#include <limits>

namespace NeoEngine {
namespace {
constexpr uint32_t kMagic = 0x454D4954U; // TIME
constexpr uint16_t kVersion = 1U;
constexpr uint32_t kMaxHostFixedTicksPerAdvance = 1000000U;
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

uint64_t Hash(std::span<const uint8_t> bytes) {
    uint64_t hash = kHashOffset;
    for (const uint8_t byte : bytes) {
        hash ^= byte;
        hash *= kHashPrime;
    }
    return hash;
}

bool ValidConfig(const RuntimeTimeConfig& config) {
    return config.gameMinutesPerFixedStep > 0U && config.gameMinutesPerFixedStep <= 1000000U &&
           config.minutesPerDay >= 2U && config.minutesPerDay <= 10000000U &&
           config.dayStartMinute < config.minutesPerDay && config.nightStartMinute < config.minutesPerDay &&
           config.dayStartMinute < config.nightStartMinute && config.defaultTimeScalePermille <= config.maxTimeScalePermille &&
           config.maxTimeScalePermille > 0U && config.maxTimeScalePermille <= RuntimeTimeSystem::kMaxTimeScalePermille &&
           config.maxEventsPerAdvance > 0U && config.maxEventsPerAdvance <= 1024U;
}
}

bool RuntimeTimeSystem::Fail(RuntimeTimeError error) {
    lastError_ = error;
    return false;
}

bool RuntimeTimeSystem::Initialize(const RuntimeTimeConfig& config) {
    if (!ValidConfig(config)) return Fail(RuntimeTimeError::InvalidConfiguration);
    config_ = config;
    snapshot_ = {};
    snapshot_.timeScalePermille = config.defaultTimeScalePermille;
    snapshot_.phase = PhaseAt(0U);
    lastReceipt_ = {};
    lastReceipt_.snapshot = snapshot_;
    eventSequence_ = 0U;
    initialized_ = true;
    lastError_ = RuntimeTimeError::None;
    return true;
}

RuntimeTimePhase RuntimeTimeSystem::PhaseAt(uint32_t minuteOfDay) const {
    return minuteOfDay >= config_.dayStartMinute && minuteOfDay < config_.nightStartMinute ? RuntimeTimePhase::Day : RuntimeTimePhase::Night;
}

RuntimeTimeSnapshot RuntimeTimeSystem::SnapshotFor(uint64_t gameTimeUnits, uint64_t hostFixedStepCount,
                                                    uint64_t stateRevision, uint16_t scalePermille, bool paused) const {
    RuntimeTimeSnapshot snapshot{};
    snapshot.gameTimeUnits = gameTimeUnits;
    snapshot.totalGameMinutes = gameTimeUnits / kUnitsPerGameMinute;
    snapshot.dayIndex = snapshot.totalGameMinutes / config_.minutesPerDay;
    snapshot.minuteOfDay = static_cast<uint32_t>(snapshot.totalGameMinutes % config_.minutesPerDay);
    snapshot.hour = static_cast<uint16_t>(snapshot.minuteOfDay / 60U);
    snapshot.minute = static_cast<uint16_t>(snapshot.minuteOfDay % 60U);
    snapshot.hostFixedStepCount = hostFixedStepCount;
    snapshot.stateRevision = stateRevision;
    snapshot.timeScalePermille = scalePermille;
    snapshot.phase = PhaseAt(snapshot.minuteOfDay);
    snapshot.paused = paused;
    return snapshot;
}

bool RuntimeTimeSystem::AppendEvent(std::vector<RuntimeTimeEvent>& events, RuntimeTimeEventKind kind,
                                    const RuntimeTimeSnapshot& snapshot, uint64_t& sequence) const {
    if (events.size() >= config_.maxEventsPerAdvance || sequence == std::numeric_limits<uint64_t>::max()) return false;
    events.push_back({kind, snapshot, ++sequence});
    return true;
}

bool RuntimeTimeSystem::AdvanceFixedTicks(uint32_t hostFixedTicks, std::vector<RuntimeTimeEvent>& events, uint32_t& simulatedTicks) {
    if (!initialized_) return Fail(RuntimeTimeError::NotInitialized);
    if (hostFixedTicks == 0U || hostFixedTicks > kMaxHostFixedTicksPerAdvance) return Fail(RuntimeTimeError::InvalidFixedTicks);
    if (snapshot_.hostFixedStepCount > std::numeric_limits<uint64_t>::max() - hostFixedTicks) return Fail(RuntimeTimeError::Overflow);

    const uint64_t oldTimeUnits = snapshot_.gameTimeUnits;
    const uint64_t oldTotalMinutes = snapshot_.totalGameMinutes;
    const uint64_t stepUnits = snapshot_.paused ? 0U : static_cast<uint64_t>(config_.gameMinutesPerFixedStep) * snapshot_.timeScalePermille;
    if (stepUnits != 0U && hostFixedTicks > std::numeric_limits<uint64_t>::max() / stepUnits) return Fail(RuntimeTimeError::Overflow);
    const uint64_t timeDeltaUnits = stepUnits * hostFixedTicks;
    if (timeDeltaUnits > std::numeric_limits<uint64_t>::max() - oldTimeUnits) return Fail(RuntimeTimeError::Overflow);
    const uint64_t newTimeUnits = oldTimeUnits + timeDeltaUnits;
    const uint64_t newHostFixedStepCount = snapshot_.hostFixedStepCount + hostFixedTicks;
    const uint64_t nextRevision = snapshot_.stateRevision == std::numeric_limits<uint64_t>::max() ? 0U : snapshot_.stateRevision + 1U;
    if (nextRevision == 0U) return Fail(RuntimeTimeError::Overflow);

    uint64_t accumulatedTicks = simulationTickAccumulatorPermille_;
    if (!snapshot_.paused) {
        if (snapshot_.timeScalePermille > 0U) {
            if (hostFixedTicks > (std::numeric_limits<uint64_t>::max() - accumulatedTicks) / snapshot_.timeScalePermille) return Fail(RuntimeTimeError::Overflow);
            accumulatedTicks += static_cast<uint64_t>(hostFixedTicks) * snapshot_.timeScalePermille;
        }
    }
    const uint64_t nextSimulatedTicks = accumulatedTicks / 1000U;
    const uint64_t candidateAccumulator = accumulatedTicks % 1000U;
    if (nextSimulatedTicks > std::numeric_limits<uint32_t>::max()) return Fail(RuntimeTimeError::Overflow);

    const RuntimeTimeSnapshot candidate = SnapshotFor(newTimeUnits, newHostFixedStepCount, nextRevision, snapshot_.timeScalePermille, snapshot_.paused);
    const bool gameTimeChanged = newTimeUnits != oldTimeUnits;
    const bool dayChanged = candidate.dayIndex != snapshot_.dayIndex;
    const bool phaseChanged = candidate.phase != snapshot_.phase;
    const uint64_t newTotalMinutes = candidate.totalGameMinutes;
    const uint64_t minuteDifference = newTotalMinutes > oldTotalMinutes ? newTotalMinutes - oldTotalMinutes : 0U;
    if (minuteDifference > std::numeric_limits<uint32_t>::max()) return Fail(RuntimeTimeError::Overflow);

    const auto CountThresholds = [&](uint32_t offset) -> uint64_t {
        if (newTotalMinutes < offset) return 0U;
        const uint64_t throughNew = (newTotalMinutes - offset) / config_.minutesPerDay + 1U;
        const uint64_t throughOld = oldTotalMinutes < offset ? 0U : (oldTotalMinutes - offset) / config_.minutesPerDay + 1U;
        return throughNew - throughOld;
    };
    const uint64_t dayTransitions = candidate.dayIndex - snapshot_.dayIndex;
    const uint64_t phaseTransitions = CountThresholds(config_.dayStartMinute) + CountThresholds(config_.nightStartMinute);
    const uint64_t requiredEvents = (gameTimeChanged ? 1U : 0U) + dayTransitions + phaseTransitions;
    if (requiredEvents > config_.maxEventsPerAdvance || requiredEvents > std::numeric_limits<size_t>::max()) return Fail(RuntimeTimeError::EventCapacity);

    struct Transition { uint64_t minute = 0U; RuntimeTimeEventKind kind = RuntimeTimeEventKind::TimeChanged; };
    std::vector<Transition> transitions;
    transitions.reserve(static_cast<size_t>(dayTransitions + phaseTransitions));
    const uint64_t firstDay = snapshot_.dayIndex;
    const uint64_t lastDay = candidate.dayIndex;
    for (uint64_t day = firstDay;; ++day) {
        const uint64_t baseMinute = day * static_cast<uint64_t>(config_.minutesPerDay);
        if (baseMinute > oldTotalMinutes && baseMinute <= newTotalMinutes) transitions.push_back({baseMinute, RuntimeTimeEventKind::DayChanged});
        const uint64_t dayMinute = baseMinute + config_.dayStartMinute;
        if (dayMinute > oldTotalMinutes && dayMinute <= newTotalMinutes) transitions.push_back({dayMinute, RuntimeTimeEventKind::PhaseChanged});
        const uint64_t nightMinute = baseMinute + config_.nightStartMinute;
        if (nightMinute > oldTotalMinutes && nightMinute <= newTotalMinutes) transitions.push_back({nightMinute, RuntimeTimeEventKind::PhaseChanged});
        if (day == lastDay) break;
    }
    std::sort(transitions.begin(), transitions.end(), [](const Transition& left, const Transition& right) {
        if (left.minute != right.minute) return left.minute < right.minute;
        return static_cast<uint8_t>(left.kind) < static_cast<uint8_t>(right.kind);
    });
    if (transitions.size() + (gameTimeChanged ? 1U : 0U) != requiredEvents) return Fail(RuntimeTimeError::EventCapacity);

    uint64_t candidateSequence = eventSequence_;
    std::vector<RuntimeTimeEvent> candidateEvents;
    candidateEvents.reserve(static_cast<size_t>(requiredEvents));
    for (const Transition& transition : transitions) {
        const uint64_t boundaryUnits = transition.minute * static_cast<uint64_t>(kUnitsPerGameMinute);
        const RuntimeTimeSnapshot boundary = SnapshotFor(boundaryUnits, newHostFixedStepCount, nextRevision, snapshot_.timeScalePermille, snapshot_.paused);
        if (!AppendEvent(candidateEvents, transition.kind, boundary, candidateSequence)) return Fail(RuntimeTimeError::EventCapacity);
    }
    if (gameTimeChanged && !AppendEvent(candidateEvents, RuntimeTimeEventKind::TimeChanged, candidate, candidateSequence)) return Fail(RuntimeTimeError::EventCapacity);

    snapshot_ = candidate;
    simulationTickAccumulatorPermille_ = candidateAccumulator;
    eventSequence_ = candidateSequence;
    simulatedTicks = static_cast<uint32_t>(nextSimulatedTicks);
    lastReceipt_ = {snapshot_, hostFixedTicks, simulatedTicks, static_cast<uint32_t>(candidateEvents.size()), gameTimeChanged, dayChanged, phaseChanged};
    events = std::move(candidateEvents);
    lastError_ = RuntimeTimeError::None;
    return true;
}

bool RuntimeTimeSystem::SetPaused(bool paused) {
    if (!initialized_) return Fail(RuntimeTimeError::NotInitialized);
    if (snapshot_.paused == paused) {
        lastError_ = RuntimeTimeError::None;
        return true;
    }
    if (snapshot_.stateRevision == std::numeric_limits<uint64_t>::max()) return Fail(RuntimeTimeError::Overflow);
    snapshot_.paused = paused;
    ++snapshot_.stateRevision;
    lastReceipt_ = {};
    lastReceipt_.snapshot = snapshot_;
    lastError_ = RuntimeTimeError::None;
    return true;
}

bool RuntimeTimeSystem::SetTimeScalePermille(uint16_t scalePermille) {
    if (!initialized_) return Fail(RuntimeTimeError::NotInitialized);
    if (scalePermille > config_.maxTimeScalePermille || scalePermille > kMaxTimeScalePermille) return Fail(RuntimeTimeError::InvalidScale);
    if (snapshot_.timeScalePermille == scalePermille) {
        lastError_ = RuntimeTimeError::None;
        return true;
    }
    if (snapshot_.stateRevision == std::numeric_limits<uint64_t>::max()) return Fail(RuntimeTimeError::Overflow);
    snapshot_.timeScalePermille = scalePermille;
    ++snapshot_.stateRevision;
    lastReceipt_ = {};
    lastReceipt_.snapshot = snapshot_;
    lastError_ = RuntimeTimeError::None;
    return true;
}

bool RuntimeTimeSystem::Serialize(std::vector<uint8_t>& bytes) const {
    if (!initialized_) return false;
    std::vector<uint8_t> candidate;
    candidate.reserve(96U);
    AppendU32(candidate, kMagic);
    AppendU16(candidate, kVersion);
    AppendU32(candidate, config_.gameMinutesPerFixedStep);
    AppendU32(candidate, config_.minutesPerDay);
    AppendU32(candidate, config_.dayStartMinute);
    AppendU32(candidate, config_.nightStartMinute);
    AppendU16(candidate, config_.defaultTimeScalePermille);
    AppendU16(candidate, config_.maxTimeScalePermille);
    AppendU16(candidate, config_.maxEventsPerAdvance);
    AppendU64(candidate, snapshot_.gameTimeUnits);
    AppendU64(candidate, snapshot_.hostFixedStepCount);
    AppendU64(candidate, snapshot_.stateRevision);
    AppendU16(candidate, snapshot_.timeScalePermille);
    candidate.push_back(snapshot_.paused ? 1U : 0U);
    AppendU64(candidate, eventSequence_);
    AppendU64(candidate, Hash(candidate));
    bytes = std::move(candidate);
    return true;
}

bool RuntimeTimeSystem::Deserialize(std::span<const uint8_t> bytes) {
    size_t offset = 0U;
    uint32_t magic = 0U, gameMinutesPerFixedStep = 0U, minutesPerDay = 0U, dayStartMinute = 0U, nightStartMinute = 0U;
    uint16_t version = 0U, defaultScale = 0U, maxScale = 0U, maxEvents = 0U, scale = 0U;
    uint64_t gameTimeUnits = 0U, hostFixedStepCount = 0U, stateRevision = 0U, eventSequence = 0U, expectedHash = 0U;
    uint8_t paused = 0U;
    if (!ReadU32(bytes, offset, magic) || !ReadU16(bytes, offset, version) || magic != kMagic || version != kVersion ||
        !ReadU32(bytes, offset, gameMinutesPerFixedStep) || !ReadU32(bytes, offset, minutesPerDay) || !ReadU32(bytes, offset, dayStartMinute) ||
        !ReadU32(bytes, offset, nightStartMinute) || !ReadU16(bytes, offset, defaultScale) || !ReadU16(bytes, offset, maxScale) ||
        !ReadU16(bytes, offset, maxEvents) || !ReadU64(bytes, offset, gameTimeUnits) || !ReadU64(bytes, offset, hostFixedStepCount) ||
        !ReadU64(bytes, offset, stateRevision) || !ReadU16(bytes, offset, scale) || offset >= bytes.size()) return Fail(RuntimeTimeError::CorruptPersistence);
    paused = bytes[offset++];
    if (!ReadU64(bytes, offset, eventSequence) || !ReadU64(bytes, offset, expectedHash) || offset != bytes.size() || paused > 1U) return Fail(RuntimeTimeError::CorruptPersistence);
    if (Hash(bytes.first(bytes.size() - sizeof(uint64_t))) != expectedHash) return Fail(RuntimeTimeError::CorruptPersistence);
    const RuntimeTimeConfig parsedConfig{gameMinutesPerFixedStep, minutesPerDay, dayStartMinute, nightStartMinute, defaultScale, maxScale, maxEvents};
    if (!ValidConfig(parsedConfig) || scale > maxScale || (stateRevision == 0U && (gameTimeUnits != 0U || hostFixedStepCount != 0U || eventSequence != 0U))) return Fail(RuntimeTimeError::CorruptPersistence);

    RuntimeTimeSystem candidate;
    candidate.config_ = parsedConfig;
    candidate.initialized_ = true;
    candidate.eventSequence_ = eventSequence;
    candidate.snapshot_ = candidate.SnapshotFor(gameTimeUnits, hostFixedStepCount, stateRevision, scale, paused != 0U);
    candidate.lastReceipt_ = {};
    candidate.lastReceipt_.snapshot = candidate.snapshot_;
    candidate.lastError_ = RuntimeTimeError::None;
    *this = std::move(candidate);
    lastError_ = RuntimeTimeError::None;
    return true;
}

} // namespace NeoEngine
