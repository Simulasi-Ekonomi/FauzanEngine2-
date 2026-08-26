#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

enum class RuntimeTimePhase : uint8_t { Night, Day };
enum class RuntimeTimeError : uint8_t {
    None,
    InvalidConfiguration,
    InvalidFixedTicks,
    InvalidScale,
    NotInitialized,
    Overflow,
    EventCapacity,
    CorruptPersistence,
};

enum class RuntimeTimeEventKind : uint8_t { TimeChanged, DayChanged, PhaseChanged };

struct RuntimeTimeConfig {
    uint32_t gameMinutesPerFixedStep = 60U;
    uint32_t minutesPerDay = 1440U;
    uint32_t dayStartMinute = 360U;
    uint32_t nightStartMinute = 1080U;
    uint16_t defaultTimeScalePermille = 1000U;
    uint16_t maxTimeScalePermille = 4000U;
    uint16_t maxEventsPerAdvance = 256U;
};

struct RuntimeTimeSnapshot {
    uint64_t gameTimeUnits = 0U; // One unit is 1/1000 of a game minute.
    uint64_t totalGameMinutes = 0U;
    uint64_t dayIndex = 0U;
    uint32_t minuteOfDay = 0U;
    uint16_t hour = 0U;
    uint16_t minute = 0U;
    uint64_t hostFixedStepCount = 0U;
    uint64_t stateRevision = 0U;
    uint16_t timeScalePermille = 1000U;
    RuntimeTimePhase phase = RuntimeTimePhase::Night;
    bool paused = false;
};

struct RuntimeTimeEvent {
    RuntimeTimeEventKind kind = RuntimeTimeEventKind::TimeChanged;
    RuntimeTimeSnapshot snapshot{};
    uint64_t sequence = 0U;
};

struct RuntimeTimeAdvanceReceipt {
    RuntimeTimeSnapshot snapshot{};
    uint32_t hostFixedTicks = 0U;
    uint32_t simulatedTicks = 0U;
    uint32_t eventCount = 0U;
    bool gameTimeChanged = false;
    bool dayChanged = false;
    bool phaseChanged = false;
};

class RuntimeTimeSystem {
public:
    static constexpr uint32_t kUnitsPerGameMinute = 1000U;
    static constexpr uint16_t kMaxTimeScalePermille = 4000U;

    bool Initialize(const RuntimeTimeConfig& config = {});
    bool AdvanceFixedTicks(uint32_t hostFixedTicks, std::vector<RuntimeTimeEvent>& events, uint32_t& simulatedTicks);
    bool SetPaused(bool paused);
    bool SetTimeScalePermille(uint16_t scalePermille);
    bool Serialize(std::vector<uint8_t>& bytes) const;
    bool Deserialize(std::span<const uint8_t> bytes);

    [[nodiscard]] bool IsReady() const { return initialized_; }
    [[nodiscard]] RuntimeTimeError LastError() const { return lastError_; }
    [[nodiscard]] const RuntimeTimeConfig& Config() const { return config_; }
    [[nodiscard]] RuntimeTimeSnapshot Snapshot() const { return snapshot_; }
    [[nodiscard]] RuntimeTimeAdvanceReceipt LastReceipt() const { return lastReceipt_; }

private:
    bool Fail(RuntimeTimeError error);
    [[nodiscard]] RuntimeTimePhase PhaseAt(uint32_t minuteOfDay) const;
    [[nodiscard]] RuntimeTimeSnapshot SnapshotFor(uint64_t gameTimeUnits, uint64_t hostFixedStepCount,
                                                  uint64_t stateRevision, uint16_t scalePermille, bool paused) const;
    bool AppendEvent(std::vector<RuntimeTimeEvent>& events, RuntimeTimeEventKind kind,
                     const RuntimeTimeSnapshot& snapshot, uint64_t& sequence) const;

    RuntimeTimeConfig config_{};
    RuntimeTimeSnapshot snapshot_{};
    RuntimeTimeAdvanceReceipt lastReceipt_{};
    uint64_t eventSequence_ = 0U;
    uint64_t simulationTickAccumulatorPermille_ = 0U;
    bool initialized_ = false;
    RuntimeTimeError lastError_ = RuntimeTimeError::None;
};

} // namespace NeoEngine
