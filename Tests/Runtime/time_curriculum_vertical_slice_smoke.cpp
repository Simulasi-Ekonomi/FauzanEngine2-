#include "Runtime/RuntimeTimeSystem.h"
#include "Systems/AgricultureCurriculum.h"
#include "Systems/CurriculumSystem.h"

#include <cstdint>
#include <vector>

int main() {
    using namespace NeoEngine;
    FarmSystem farm(6U, 2U, 100000);
    if (!farm.IsReady()) return 1;
    CurriculumGraph graph;
    if (!BuildAgricultureCurriculum(graph) || !graph.IsReady()) return 2;
    CurriculumSystem curriculum;
    if (!curriculum.Initialize(graph)) return 3;

    CurriculumObservation observation{};
    std::vector<CurriculumEvent> events;
    if (!curriculum.Evaluate(observation, events)) return 4;
    CurriculumProgressReceipt receipt{};
    if (!curriculum.Snapshot(receipt) || receipt.completedLessons != 0U) return 5;

    for (uint16_t x = 0U; x < 5U; ++x) if (!farm.Till(x, 0U) || !farm.Plant(x, 0U, FarmCrop::Wheat) || !farm.Water(x, 0U)) return 6;
    if (!farm.AddAnimal(FarmAnimal::Hen)) return 7;
    observation.farm = farm.Snapshot();
    if (!curriculum.Evaluate(observation, events) || receipt.completedLessons != 0U) return 8;
    if (!curriculum.Snapshot(receipt) || receipt.completedLessons < 3U) return 9;

    RuntimeTimeSystem time;
    if (!time.Initialize({60U, 1440U, 360U, 1080U, 1000U, 4000U, 32U})) return 10;
    std::vector<RuntimeTimeEvent> timeEvents;
    uint32_t simulatedTicks = 0U;
    if (!time.AdvanceFixedTicks(48U, timeEvents, simulatedTicks) || simulatedTicks != 48U || time.Snapshot().totalGameMinutes != 2880U || time.Snapshot().dayIndex != 2U) return 11;
    if (!farm.Tick(simulatedTicks)) return 12;
    observation.time = time.Snapshot();
    observation.farm = farm.Snapshot();
    if (!curriculum.Evaluate(observation, events)) return 13;
    if (!curriculum.Snapshot(receipt) || receipt.completedLessons != 4U || receipt.newlyEarnedRewards.empty()) return 14;

    uint32_t harvested = 0U;
    if (!farm.Harvest(0U, 0U, harvested) || harvested != 2U) return 15;
    observation.farm = farm.Snapshot();
    if (!curriculum.Evaluate(observation, events)) return 16;
    if (!curriculum.Snapshot(receipt) || receipt.completedLessons < 7U) return 17;
    LessonProgress jagung{};
    if (!curriculum.Query("agri.jagung-value-chain", jagung) || jagung.status != LessonStatus::Completed || jagung.completedConditions != 1U) return 18;

    for (uint16_t x = 1U; x < 5U; ++x) if (!farm.Harvest(x, 0U, harvested) || harvested != 2U) return 19;
    observation.farm = farm.Snapshot();
    if (!observation.farm.questCompleted || !curriculum.Evaluate(observation, events)) return 20;
    if (!curriculum.Snapshot(receipt) || receipt.completedLessons != 8U) return 21;
    LessonProgress enterprise{};
    if (!curriculum.Query("agri.enterprise-readiness", enterprise) || enterprise.status != LessonStatus::Completed) return 22;

    std::vector<uint8_t> saved;
    if (!curriculum.Serialize(saved) || saved.empty()) return 23;
    CurriculumSystem restored;
    if (!restored.Initialize(graph) || !restored.Deserialize(saved)) return 24;
    CurriculumProgressReceipt restoredReceipt{};
    if (!restored.Snapshot(restoredReceipt) || restoredReceipt.completedLessons != receipt.completedLessons || restoredReceipt.revision != receipt.revision) return 25;
    const uint64_t preservedRevision = restoredReceipt.revision;
    std::vector<uint8_t> corrupt = saved;
    corrupt.back() ^= 0x5AU;
    if (restored.Deserialize(corrupt) || restored.LastError() != CurriculumError::CorruptPersistence) return 26;
    if (restored.LastReceipt().revision != preservedRevision) return 27;
    return 0;
}
