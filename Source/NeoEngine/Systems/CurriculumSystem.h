#pragma once

#include "FarmSystem.h"
#include "Runtime/RuntimeTimeSystem.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace NeoEngine {

enum class LessonStatus : uint8_t { Locked, Available, InProgress, Completed };
enum class LessonConditionKind : uint8_t {
    GameMinutesAtLeast,
    DayIndexAtLeast,
    SimulationTicksAtLeast,
    TilledTilesAtLeast,
    GrowingTilesAtLeast,
    HarvestActionsAtLeast,
    HarvestedUnitsAtLeast,
    CoinsAtLeast,
    AnimalsAtLeast,
    FarmQuestCompleted,
};
enum class CurriculumError : uint8_t {
    None,
    InvalidConfiguration,
    InvalidId,
    DuplicateId,
    MissingPrerequisite,
    CycleDetected,
    Capacity,
    InvalidCondition,
    NotInitialized,
    CorruptPersistence,
    RevisionOverflow,
};

enum class LessonMaterialKind : uint8_t { Explanation, Table, Rule, SourceNote };
struct LessonMaterial {
    LessonMaterialKind kind = LessonMaterialKind::Explanation;
    std::string title;
    std::string body;
    std::string sourceDocument;
};

struct LessonReward {
    std::string flag;
    int64_t amount = 0;
};

struct LessonCondition {
    LessonConditionKind kind = LessonConditionKind::GameMinutesAtLeast;
    uint64_t target = 0U;
};

struct LessonNode {
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> prerequisites;
    std::vector<LessonCondition> completionConditions;
    std::vector<LessonMaterial> materials;
    std::vector<LessonReward> rewards;
};

struct CurriculumConfig {
    uint16_t maxLessons = 128U;
    uint16_t maxPrerequisitesPerLesson = 16U;
    uint16_t maxConditionsPerLesson = 16U;
    uint16_t maxMaterialsPerLesson = 16U;
    uint16_t maxRewardsPerLesson = 16U;
};

class CurriculumGraph {
public:
    bool Initialize(const CurriculumConfig& config = {});
    bool AddLesson(LessonNode lesson);
    bool Finalize();
    [[nodiscard]] bool IsReady() const { return ready_; }
    [[nodiscard]] CurriculumError LastError() const { return lastError_; }
    [[nodiscard]] const LessonNode* Find(std::string_view id) const;
    [[nodiscard]] const std::vector<LessonNode>& Lessons() const { return lessons_; }
    [[nodiscard]] const std::vector<uint16_t>& EvaluationOrder() const { return evaluationOrder_; }

private:
    bool Fail(CurriculumError error);
    CurriculumConfig config_{};
    std::vector<LessonNode> lessons_{};
    std::vector<uint16_t> evaluationOrder_{};
    bool ready_ = false;
    CurriculumError lastError_ = CurriculumError::None;
};

struct CurriculumObservation {
    RuntimeTimeSnapshot time{};
    FarmTelemetrySnapshot farm{};
};

struct LessonProgress {
    std::string id;
    LessonStatus status = LessonStatus::Locked;
    uint16_t completedConditions = 0U;
    uint16_t totalConditions = 0U;
    uint64_t completedAtGameMinutes = 0U;
    uint64_t completionRevision = 0U;
};

struct CurriculumProgressReceipt {
    uint64_t revision = 0U;
    uint16_t completedLessons = 0U;
    uint16_t availableLessons = 0U;
    uint16_t inProgressLessons = 0U;
    std::vector<LessonProgress> lessons{};
    std::vector<LessonReward> newlyEarnedRewards{};
};

struct CurriculumEvent {
    std::string lessonId;
    LessonStatus status = LessonStatus::Locked;
    uint64_t revision = 0U;
};

class CurriculumSystem {
public:
    static constexpr uint32_t kMaxSerializedBytes = 1024U * 1024U;
    bool Initialize(const CurriculumGraph& graph);
    bool Evaluate(const CurriculumObservation& observation, std::vector<CurriculumEvent>& events);
    bool Query(std::string_view lessonId, LessonProgress& progress) const;
    bool Snapshot(CurriculumProgressReceipt& receipt) const;
    bool Serialize(std::vector<uint8_t>& bytes) const;
    bool Deserialize(std::span<const uint8_t> bytes);

    [[nodiscard]] bool IsReady() const { return initialized_; }
    [[nodiscard]] CurriculumError LastError() const { return lastError_; }
    [[nodiscard]] const CurriculumProgressReceipt& LastReceipt() const { return lastReceipt_; }
    [[nodiscard]] const CurriculumGraph* Graph() const { return graph_; }

private:
    bool Fail(CurriculumError error);
    bool EvaluateCondition(const LessonCondition& condition, const CurriculumObservation& observation) const;
    bool PrerequisitesCompleted(uint16_t lessonIndex, const std::vector<uint8_t>& completed) const;
    bool BuildReceipt(const CurriculumObservation& observation, CurriculumProgressReceipt& receipt) const;
    bool ValidateProgress(const std::vector<uint8_t>& completed, uint64_t revision) const;

    const CurriculumGraph* graph_ = nullptr;
    std::vector<uint8_t> completed_{};
    std::vector<uint64_t> completedAtGameMinutes_{};
    std::vector<uint64_t> completionRevisions_{};
    CurriculumProgressReceipt lastReceipt_{};
    uint64_t revision_ = 0U;
    bool initialized_ = false;
    CurriculumError lastError_ = CurriculumError::None;
};

} // namespace NeoEngine
