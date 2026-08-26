#include "Systems/AgricultureCurriculum.h"
#include "Systems/CurriculumSystem.h"

#include <cstdint>
#include <vector>

namespace {
NeoEngine::LessonNode Lesson(const char* id, std::vector<std::string> prerequisites = {}) {
    return {id, "Lesson", "Valid lesson", std::move(prerequisites), {{NeoEngine::LessonConditionKind::GameMinutesAtLeast, 1U}}, {{NeoEngine::LessonMaterialKind::Explanation, "Material", "Valid material", "test"}}, {}};
}
}

int main() {
    using namespace NeoEngine;
    {
        CurriculumGraph graph;
        if (!graph.Initialize() || !graph.AddLesson(Lesson("duplicate")) || graph.AddLesson(Lesson("duplicate")) || graph.LastError() != CurriculumError::DuplicateId) return 1;
    }
    {
        CurriculumGraph graph;
        if (!graph.Initialize() || !graph.AddLesson(Lesson("missing", {"not-present"})) || graph.Finalize() || graph.LastError() != CurriculumError::MissingPrerequisite) return 2;
    }
    {
        CurriculumGraph graph;
        if (!graph.Initialize() || !graph.AddLesson(Lesson("a", {"b"})) || !graph.AddLesson(Lesson("b", {"a"})) || graph.Finalize() || graph.LastError() != CurriculumError::CycleDetected) return 3;
    }
    {
        CurriculumGraph graph;
        LessonNode invalid = Lesson("invalid");
        invalid.completionConditions[0].kind = static_cast<LessonConditionKind>(255U);
        if (!graph.Initialize() || graph.AddLesson(std::move(invalid)) || graph.LastError() != CurriculumError::InvalidCondition) return 4;
    }

    CurriculumSystem curriculum;
    {
        CurriculumGraph temporary;
        if (!BuildAgricultureCurriculum(temporary) || !curriculum.Initialize(temporary)) return 5;
    }
    if (curriculum.Graph() == nullptr || !curriculum.Graph()->IsReady()) return 6;
    CurriculumObservation observation{};
    observation.time.totalGameMinutes = 60U;
    observation.farm.growingTiles = 1U;
    observation.farm.animals = 1U;
    observation.farm.coins = 100000;
    std::vector<CurriculumEvent> events;
    if (!curriculum.Evaluate(observation, events) || events.size() != 3U || curriculum.LastReceipt().completedLessons != 3U || curriculum.LastReceipt().revision != 1U) return 7;
    const uint64_t stableRevision = curriculum.LastReceipt().revision;
    if (!curriculum.Evaluate(observation, events) || !events.empty() || curriculum.LastReceipt().revision != stableRevision) return 8;
    LessonProgress progress{};
    if (!curriculum.Query("agri.orientation", progress) || progress.status != LessonStatus::Completed || curriculum.Query("bad/id", progress) || curriculum.Query("unknown", progress)) return 9;

    std::vector<uint8_t> saved;
    if (!curriculum.Serialize(saved) || saved.empty()) return 10;
    const CurriculumProgressReceipt preserved = curriculum.LastReceipt();
    std::vector<uint8_t> corrupt = saved;
    corrupt[0] ^= 0xFFU;
    if (curriculum.Deserialize(corrupt) || curriculum.LastError() != CurriculumError::CorruptPersistence || curriculum.LastReceipt().revision != preserved.revision || curriculum.LastReceipt().completedLessons != preserved.completedLessons) return 11;
    return 0;
}
