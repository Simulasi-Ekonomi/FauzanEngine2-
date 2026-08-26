#include "CurriculumSystem.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>

namespace NeoEngine {
namespace {
constexpr uint32_t kMagic = 0x52525543U; // CURR
constexpr uint16_t kVersion = 1U;
constexpr size_t kMaxTextBytes = 4096U;
constexpr uint64_t kHashOffset = 1469598103934665603ULL;
constexpr uint64_t kHashPrime = 1099511628211ULL;

bool ValidId(std::string_view value) {
    if (value.empty() || value.size() > 96U) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_' || c == '.'; });
}

bool ValidText(std::string_view value, size_t maxBytes = kMaxTextBytes) {
    return !value.empty() && value.size() <= maxBytes && std::all_of(value.begin(), value.end(), [](unsigned char c) { return c >= 0x20U && c != 0x7FU; });
}

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
    for (const uint8_t byte : bytes) { hash ^= byte; hash *= kHashPrime; }
    return hash;
}
bool ReadString(std::span<const uint8_t> bytes, size_t& offset, std::string& value) {
    uint16_t length = 0U;
    if (!ReadU16(bytes, offset, length) || length == 0U || length > kMaxTextBytes || offset > bytes.size() || bytes.size() - offset < length) return false;
    value.assign(reinterpret_cast<const char*>(bytes.data() + offset), length);
    offset += length;
    return ValidText(value);
}
void AppendString(std::vector<uint8_t>& bytes, std::string_view value) {
    AppendU16(bytes, static_cast<uint16_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
}
size_t FindIndex(const std::vector<LessonNode>& lessons, std::string_view id) {
    const auto found = std::find_if(lessons.begin(), lessons.end(), [id](const LessonNode& lesson) { return lesson.id == id; });
    return found == lessons.end() ? lessons.size() : static_cast<size_t>(std::distance(lessons.begin(), found));
}
uint64_t GraphFingerprint(const CurriculumGraph& graph) {
    uint64_t hash = kHashOffset;
    for (const LessonNode& lesson : graph.Lessons()) {
        for (const unsigned char c : lesson.id) { hash ^= c; hash *= kHashPrime; }
        for (const std::string& prerequisite : lesson.prerequisites) for (const unsigned char c : prerequisite) { hash ^= c; hash *= kHashPrime; }
        for (const LessonCondition& condition : lesson.completionConditions) { hash ^= static_cast<uint8_t>(condition.kind); hash *= kHashPrime; hash ^= condition.target; hash *= kHashPrime; }
    }
    return hash;
}
}

bool CurriculumGraph::Fail(CurriculumError error) { lastError_ = error; return false; }

bool CurriculumGraph::Initialize(const CurriculumConfig& config) {
    if (config.maxLessons == 0U || config.maxLessons > 4096U || config.maxPrerequisitesPerLesson == 0U || config.maxConditionsPerLesson == 0U || config.maxMaterialsPerLesson == 0U || config.maxRewardsPerLesson == 0U) return Fail(CurriculumError::InvalidConfiguration);
    config_ = config;
    lessons_.clear();
    evaluationOrder_.clear();
    ready_ = false;
    lastError_ = CurriculumError::None;
    return true;
}

bool CurriculumGraph::AddLesson(LessonNode lesson) {
    if (ready_) return Fail(CurriculumError::InvalidConfiguration);
    if (lessons_.size() >= config_.maxLessons || !ValidId(lesson.id) || !ValidText(lesson.name, 256U) || !ValidText(lesson.description, 2048U)) return Fail(CurriculumError::InvalidConfiguration);
    if (lesson.prerequisites.size() > config_.maxPrerequisitesPerLesson || lesson.completionConditions.empty() || lesson.completionConditions.size() > config_.maxConditionsPerLesson || lesson.materials.size() > config_.maxMaterialsPerLesson || lesson.rewards.size() > config_.maxRewardsPerLesson) return Fail(CurriculumError::Capacity);
    if (FindIndex(lessons_, lesson.id) != lessons_.size()) return Fail(CurriculumError::DuplicateId);
    for (size_t prerequisiteIndex = 0U; prerequisiteIndex < lesson.prerequisites.size(); ++prerequisiteIndex) {
        const std::string& prerequisite = lesson.prerequisites[prerequisiteIndex];
        if (!ValidId(prerequisite) || prerequisite == lesson.id || std::find(lesson.prerequisites.begin(), lesson.prerequisites.begin() + static_cast<std::ptrdiff_t>(prerequisiteIndex), prerequisite) != lesson.prerequisites.begin() + static_cast<std::ptrdiff_t>(prerequisiteIndex)) return Fail(CurriculumError::InvalidId);
    }
    for (const LessonCondition& condition : lesson.completionConditions) if (condition.target == 0U || static_cast<uint8_t>(condition.kind) > static_cast<uint8_t>(LessonConditionKind::FarmQuestCompleted)) return Fail(CurriculumError::InvalidCondition);
    for (const LessonMaterial& material : lesson.materials) if (!ValidText(material.title, 256U) || !ValidText(material.body) || !ValidText(material.sourceDocument, 256U)) return Fail(CurriculumError::InvalidConfiguration);
    for (const LessonReward& reward : lesson.rewards) if (!ValidId(reward.flag) || reward.amount < 0) return Fail(CurriculumError::InvalidConfiguration);
    lessons_.push_back(std::move(lesson));
    lastError_ = CurriculumError::None;
    return true;
}

bool CurriculumGraph::Finalize() {
    if (lessons_.empty()) return Fail(CurriculumError::InvalidConfiguration);
    std::vector<uint16_t> indegree(lessons_.size(), 0U);
    std::vector<std::vector<uint16_t>> edges(lessons_.size());
    for (size_t index = 0U; index < lessons_.size(); ++index) {
        for (const std::string& prerequisite : lessons_[index].prerequisites) {
            const size_t parent = FindIndex(lessons_, prerequisite);
            if (parent == lessons_.size()) return Fail(CurriculumError::MissingPrerequisite);
            edges[parent].push_back(static_cast<uint16_t>(index));
            ++indegree[index];
        }
    }
    std::vector<uint16_t> candidate;
    candidate.reserve(lessons_.size());
    for (uint16_t index = 0U; index < lessons_.size(); ++index) if (indegree[index] == 0U) candidate.push_back(index);
    for (size_t cursor = 0U; cursor < candidate.size(); ++cursor) {
        const uint16_t index = candidate[cursor];
        for (const uint16_t child : edges[index]) if (--indegree[child] == 0U) candidate.push_back(child);
    }
    if (candidate.size() != lessons_.size()) return Fail(CurriculumError::CycleDetected);
    evaluationOrder_ = std::move(candidate);
    ready_ = true;
    lastError_ = CurriculumError::None;
    return true;
}

const LessonNode* CurriculumGraph::Find(std::string_view id) const {
    const size_t index = FindIndex(lessons_, id);
    return index == lessons_.size() ? nullptr : &lessons_[index];
}

bool CurriculumSystem::Fail(CurriculumError error) { lastError_ = error; return false; }

bool CurriculumSystem::Initialize(const CurriculumGraph& graph) {
    if (!graph.IsReady()) return Fail(CurriculumError::InvalidConfiguration);
    graph_ = &graph;
    completed_.assign(graph.Lessons().size(), 0U);
    completedAtGameMinutes_.assign(graph.Lessons().size(), 0U);
    completionRevisions_.assign(graph.Lessons().size(), 0U);
    lastReceipt_ = {};
    lastReceipt_.lessons.reserve(graph.Lessons().size());
    revision_ = 0U;
    initialized_ = true;
    CurriculumObservation initialObservation{};
    if (!BuildReceipt(initialObservation, lastReceipt_)) { initialized_ = false; graph_ = nullptr; return Fail(CurriculumError::InvalidConfiguration); }
    lastError_ = CurriculumError::None;
    return true;
}

bool CurriculumSystem::EvaluateCondition(const LessonCondition& condition, const CurriculumObservation& observation) const {
    switch (condition.kind) {
        case LessonConditionKind::GameMinutesAtLeast: return observation.time.totalGameMinutes >= condition.target;
        case LessonConditionKind::DayIndexAtLeast: return observation.time.dayIndex >= condition.target;
        case LessonConditionKind::SimulationTicksAtLeast: return observation.farm.simulationTick >= condition.target;
        case LessonConditionKind::TilledTilesAtLeast: return observation.farm.tilledTiles >= condition.target;
        case LessonConditionKind::GrowingTilesAtLeast: return observation.farm.growingTiles >= condition.target;
        case LessonConditionKind::HarvestActionsAtLeast: return observation.farm.harvestActions >= condition.target;
        case LessonConditionKind::HarvestedUnitsAtLeast: return observation.farm.harvestedUnits >= condition.target;
        case LessonConditionKind::CoinsAtLeast: return observation.farm.coins >= 0 && static_cast<uint64_t>(observation.farm.coins) >= condition.target;
        case LessonConditionKind::AnimalsAtLeast: return observation.farm.animals >= condition.target;
        case LessonConditionKind::FarmQuestCompleted: return observation.farm.questCompleted;
    }
    return false;
}

bool CurriculumSystem::PrerequisitesCompleted(uint16_t lessonIndex, const std::vector<uint8_t>& completed) const {
    const LessonNode& lesson = graph_->Lessons()[lessonIndex];
    for (const std::string& prerequisite : lesson.prerequisites) {
        const size_t index = FindIndex(graph_->Lessons(), prerequisite);
        if (index == graph_->Lessons().size() || completed[index] == 0U) return false;
    }
    return true;
}

bool CurriculumSystem::BuildReceipt(const CurriculumObservation& observation, CurriculumProgressReceipt& receipt) const {
    if (!initialized_ || graph_ == nullptr || completed_.size() != graph_->Lessons().size()) return false;
    CurriculumProgressReceipt candidate{};
    candidate.revision = revision_;
    candidate.lessons.reserve(graph_->Lessons().size());
    for (uint16_t index = 0U; index < graph_->Lessons().size(); ++index) {
        const LessonNode& lesson = graph_->Lessons()[index];
        LessonProgress progress{};
        progress.id = lesson.id;
        progress.totalConditions = static_cast<uint16_t>(lesson.completionConditions.size());
        progress.completedConditions = static_cast<uint16_t>(std::count_if(lesson.completionConditions.begin(), lesson.completionConditions.end(), [this, &observation](const LessonCondition& condition) { return EvaluateCondition(condition, observation); }));
        progress.completedAtGameMinutes = completedAtGameMinutes_[index];
        progress.completionRevision = completionRevisions_[index];
        if (completed_[index] != 0U) progress.status = LessonStatus::Completed;
        else if (!PrerequisitesCompleted(index, completed_)) progress.status = LessonStatus::Locked;
        else if (progress.completedConditions == progress.totalConditions) progress.status = LessonStatus::InProgress;
        else if (progress.completedConditions != 0U) progress.status = LessonStatus::InProgress;
        else progress.status = LessonStatus::Available;
        if (progress.status == LessonStatus::Completed) ++candidate.completedLessons;
        else if (progress.status == LessonStatus::Available) ++candidate.availableLessons;
        else if (progress.status == LessonStatus::InProgress) ++candidate.inProgressLessons;
        candidate.lessons.push_back(std::move(progress));
    }
    receipt = std::move(candidate);
    return true;
}

bool CurriculumSystem::Evaluate(const CurriculumObservation& observation, std::vector<CurriculumEvent>& events) {
    if (!initialized_ || graph_ == nullptr) return Fail(CurriculumError::NotInitialized);
    std::vector<uint8_t> candidateCompleted = completed_;
    std::vector<uint64_t> candidateCompletedAt = completedAtGameMinutes_;
    std::vector<uint64_t> candidateCompletionRevisions = completionRevisions_;
    bool changed = false;
    uint64_t candidateRevision = revision_;
    if (candidateRevision == std::numeric_limits<uint64_t>::max()) return Fail(CurriculumError::RevisionOverflow);
    for (const uint16_t index : graph_->EvaluationOrder()) {
        if (candidateCompleted[index] != 0U || !PrerequisitesCompleted(index, candidateCompleted)) continue;
        const LessonNode& lesson = graph_->Lessons()[index];
        const bool allConditions = std::all_of(lesson.completionConditions.begin(), lesson.completionConditions.end(), [this, &observation](const LessonCondition& condition) { return EvaluateCondition(condition, observation); });
        if (allConditions) {
            candidateCompleted[index] = 1U;
            changed = true;
        }
    }
    if (changed) {
        candidateRevision = revision_ + 1U;
        for (uint16_t index = 0U; index < graph_->Lessons().size(); ++index) {
            if (completed_[index] == 0U && candidateCompleted[index] != 0U) {
                candidateCompletedAt[index] = observation.time.totalGameMinutes;
                candidateCompletionRevisions[index] = candidateRevision;
            }
        }
    }
    const std::vector<uint64_t> previousCompletionRevisions = completionRevisions_;
    completionRevisions_ = candidateCompletionRevisions;
    const bool validCandidate = ValidateProgress(candidateCompleted, candidateRevision);
    completionRevisions_ = previousCompletionRevisions;
    if (!validCandidate) return Fail(CurriculumError::CorruptPersistence);

    const std::vector<uint8_t> oldCompleted = completed_;
    completed_ = std::move(candidateCompleted);
    completedAtGameMinutes_ = std::move(candidateCompletedAt);
    completionRevisions_ = std::move(candidateCompletionRevisions);
    revision_ = candidateRevision;
    CurriculumProgressReceipt candidateReceipt{};
    if (!BuildReceipt(observation, candidateReceipt)) return Fail(CurriculumError::CorruptPersistence);
    events.clear();
    for (uint16_t index = 0U; index < graph_->Lessons().size(); ++index) {
        if (oldCompleted[index] == 0U && completed_[index] != 0U) {
            events.push_back({graph_->Lessons()[index].id, LessonStatus::Completed, revision_});
            candidateReceipt.newlyEarnedRewards.insert(candidateReceipt.newlyEarnedRewards.end(), graph_->Lessons()[index].rewards.begin(), graph_->Lessons()[index].rewards.end());
        }
    }
    lastReceipt_ = std::move(candidateReceipt);
    lastError_ = CurriculumError::None;
    return true;
}

bool CurriculumSystem::Query(std::string_view lessonId, LessonProgress& progress) const {
    if (!initialized_ || !ValidId(lessonId)) return false;
    const auto found = std::find_if(lastReceipt_.lessons.begin(), lastReceipt_.lessons.end(), [lessonId](const LessonProgress& item) { return item.id == lessonId; });
    if (found == lastReceipt_.lessons.end()) return false;
    progress = *found;
    return true;
}

bool CurriculumSystem::Snapshot(CurriculumProgressReceipt& receipt) const {
    if (!initialized_) return false;
    receipt = lastReceipt_;
    return true;
}

bool CurriculumSystem::ValidateProgress(const std::vector<uint8_t>& completed, uint64_t revision) const {
    if (graph_ == nullptr || completed.size() != graph_->Lessons().size()) return false;
    for (uint16_t index = 0U; index < completed.size(); ++index) {
        if (completed[index] > 1U) return false;
        if (completed[index] != 0U && !PrerequisitesCompleted(index, completed)) return false;
        if (completed[index] != 0U && completionRevisions_[index] > revision) return false;
    }
    return true;
}

bool CurriculumSystem::Serialize(std::vector<uint8_t>& bytes) const {
    if (!initialized_ || graph_ == nullptr || completed_.size() > std::numeric_limits<uint16_t>::max()) return false;
    std::vector<uint8_t> candidate;
    AppendU32(candidate, kMagic);
    AppendU16(candidate, kVersion);
    AppendU64(candidate, GraphFingerprint(*graph_));
    AppendU64(candidate, revision_);
    AppendU16(candidate, static_cast<uint16_t>(completed_.size()));
    for (uint16_t index = 0U; index < completed_.size(); ++index) {
        candidate.push_back(completed_[index]);
        AppendU64(candidate, completedAtGameMinutes_[index]);
        AppendU64(candidate, completionRevisions_[index]);
    }
    AppendU64(candidate, Hash(candidate));
    if (candidate.size() > kMaxSerializedBytes) return false;
    bytes = std::move(candidate);
    return true;
}

bool CurriculumSystem::Deserialize(std::span<const uint8_t> bytes) {
    if (!initialized_ || graph_ == nullptr) return Fail(CurriculumError::NotInitialized);
    if (bytes.size() > kMaxSerializedBytes) return Fail(CurriculumError::CorruptPersistence);
    size_t offset = 0U;
    uint32_t magic = 0U;
    uint16_t version = 0U, count = 0U;
    uint64_t graphFingerprint = 0U, revision = 0U, expectedHash = 0U;
    if (!ReadU32(bytes, offset, magic) || !ReadU16(bytes, offset, version) || !ReadU64(bytes, offset, graphFingerprint) || !ReadU64(bytes, offset, revision) || !ReadU16(bytes, offset, count) || magic != kMagic || version != kVersion || graphFingerprint != GraphFingerprint(*graph_) || count != graph_->Lessons().size()) return Fail(CurriculumError::CorruptPersistence);
    std::vector<uint8_t> candidateCompleted(count, 0U);
    std::vector<uint64_t> candidateCompletedAt(count, 0U);
    std::vector<uint64_t> candidateCompletionRevisions(count, 0U);
    for (uint16_t index = 0U; index < count; ++index) {
        if (offset >= bytes.size() || bytes[offset] > 1U) return Fail(CurriculumError::CorruptPersistence);
        candidateCompleted[index] = bytes[offset++];
        if (!ReadU64(bytes, offset, candidateCompletedAt[index]) || !ReadU64(bytes, offset, candidateCompletionRevisions[index])) return Fail(CurriculumError::CorruptPersistence);
    }
    if (!ReadU64(bytes, offset, expectedHash) || offset != bytes.size() || Hash(bytes.first(bytes.size() - sizeof(uint64_t))) != expectedHash) return Fail(CurriculumError::CorruptPersistence);
    const std::vector<uint64_t> oldCompletionRevisions = completionRevisions_;
    completionRevisions_ = candidateCompletionRevisions;
    const bool valid = ValidateProgress(candidateCompleted, revision);
    completionRevisions_ = oldCompletionRevisions;
    if (!valid) return Fail(CurriculumError::CorruptPersistence);
    completed_ = std::move(candidateCompleted);
    completedAtGameMinutes_ = std::move(candidateCompletedAt);
    completionRevisions_ = std::move(candidateCompletionRevisions);
    revision_ = revision;
    CurriculumObservation restoredObservation{};
    if (!BuildReceipt(restoredObservation, lastReceipt_)) return Fail(CurriculumError::CorruptPersistence);
    lastError_ = CurriculumError::None;
    return true;
}

} // namespace NeoEngine
