#include "Runtime/RuntimeTimeSystem.h"

#include <algorithm>
#include <cstdint>
#include <vector>

int main() {
    using namespace NeoEngine;
    RuntimeTimeSystem time;
    const RuntimeTimeConfig config{60U, 1440U, 360U, 1080U, 1000U, 4000U, 16U};
    if (!time.Initialize(config)) return 1;
    if (time.Snapshot().dayIndex != 0U || time.Snapshot().minuteOfDay != 0U || time.Snapshot().phase != RuntimeTimePhase::Night) return 2;

    std::vector<RuntimeTimeEvent> events;
    uint32_t simulatedTicks = 0U;
    if (!time.AdvanceFixedTicks(6U, events, simulatedTicks)) return 3;
    if (simulatedTicks != 6U || time.Snapshot().totalGameMinutes != 360U || time.Snapshot().phase != RuntimeTimePhase::Day || events.size() != 2U) return 4;
    if (events[0].kind != RuntimeTimeEventKind::PhaseChanged || events[0].snapshot.minuteOfDay != 360U || events[1].kind != RuntimeTimeEventKind::TimeChanged || events[1].snapshot.minuteOfDay != 360U) return 5;

    if (!time.AdvanceFixedTicks(18U, events, simulatedTicks)) return 6;
    if (simulatedTicks != 18U || time.Snapshot().dayIndex != 1U || time.Snapshot().minuteOfDay != 0U || time.Snapshot().phase != RuntimeTimePhase::Night || events.size() != 3U) return 7;
    if (events[0].kind != RuntimeTimeEventKind::PhaseChanged || events[0].snapshot.minuteOfDay != 1080U || events[1].kind != RuntimeTimeEventKind::DayChanged || events[2].kind != RuntimeTimeEventKind::TimeChanged) return 8;

    RuntimeTimeSystem batch;
    if (!batch.Initialize({60U, 1440U, 360U, 1080U, 1000U, 4000U, 16U}) || !batch.AdvanceFixedTicks(48U, events, simulatedTicks) || events.size() != 7U || events[0].kind != RuntimeTimeEventKind::PhaseChanged || events[0].snapshot.totalGameMinutes != 360U || events[5].kind != RuntimeTimeEventKind::DayChanged || events[5].snapshot.totalGameMinutes != 2880U || events[6].kind != RuntimeTimeEventKind::TimeChanged) return 9;
    RuntimeTimeSystem bounded;
    if (!bounded.Initialize({60U, 1440U, 360U, 1080U, 1000U, 4000U, 4U})) return 10;
    const RuntimeTimeSnapshot boundedBefore = bounded.Snapshot();
    if (bounded.AdvanceFixedTicks(48U, events, simulatedTicks) || bounded.LastError() != RuntimeTimeError::EventCapacity || bounded.Snapshot().gameTimeUnits != boundedBefore.gameTimeUnits || bounded.Snapshot().hostFixedStepCount != boundedBefore.hostFixedStepCount) return 11;

    if (!time.SetTimeScalePermille(500U)) return 12;
    const uint64_t beforeHalfScale = time.Snapshot().gameTimeUnits;
    if (!time.AdvanceFixedTicks(1U, events, simulatedTicks) || simulatedTicks != 0U || time.Snapshot().gameTimeUnits - beforeHalfScale != 30000U) return 13;
    if (!time.AdvanceFixedTicks(1U, events, simulatedTicks) || simulatedTicks != 1U || time.Snapshot().gameTimeUnits - beforeHalfScale != 60000U) return 14;

    const RuntimeTimeSnapshot beforePause = time.Snapshot();
    if (!time.SetPaused(true) || !time.AdvanceFixedTicks(7U, events, simulatedTicks) || simulatedTicks != 0U || time.Snapshot().gameTimeUnits != beforePause.gameTimeUnits) return 15;
    if (!time.SetPaused(false) || !time.AdvanceFixedTicks(2U, events, simulatedTicks) || simulatedTicks != 1U) return 16;
    if (time.SetTimeScalePermille(4001U) || time.LastError() != RuntimeTimeError::InvalidScale) return 17;

    std::vector<uint8_t> saved;
    if (!time.Serialize(saved) || saved.empty()) return 18;
    RuntimeTimeSystem restored;
    if (!restored.Deserialize(saved) || restored.Snapshot().gameTimeUnits != time.Snapshot().gameTimeUnits || restored.Snapshot().hostFixedStepCount != time.Snapshot().hostFixedStepCount || restored.Snapshot().timeScalePermille != time.Snapshot().timeScalePermille) return 19;
    const RuntimeTimeSnapshot preserved = restored.Snapshot();
    std::vector<uint8_t> corrupted = saved;
    corrupted.back() ^= 0xA5U;
    if (restored.Deserialize(corrupted) || restored.LastError() != RuntimeTimeError::CorruptPersistence || restored.Snapshot().gameTimeUnits != preserved.gameTimeUnits) return 20;

    RuntimeTimeSystem invalid;
    if (invalid.AdvanceFixedTicks(1U, events, simulatedTicks) || invalid.LastError() != RuntimeTimeError::NotInitialized) return 21;
    return 0;
}
