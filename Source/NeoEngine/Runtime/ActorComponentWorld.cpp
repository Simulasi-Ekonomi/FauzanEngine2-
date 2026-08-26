#include "ActorComponentWorld.h"
#include <limits>
#include <new>
namespace NeoEngine {
ActorComponentWorld::ActorComponentWorld(SceneWorld& sceneWorld) : sceneWorld_(sceneWorld) {}
ActorComponentWorld::~ActorComponentWorld() {
    for (uint16_t index = 0U; index < kCapacity; ++index) {
        if (!actors_[index].registered) continue;
        ActorSlot& actor = actors_[index];
        if (actor.begunPlay) EndActorPlay(actor);
        for (uint8_t componentIndex = 0U; componentIndex < kMaxComponentsPerActor; ++componentIndex) {
            ComponentSlot& slot = actor.components[componentIndex];
            if (slot.component != nullptr) slot.component->OnDetach(sceneWorld_, actor.scene);
        }
    }
}
bool ActorComponentWorld::Fail(ActorComponentError error) const {
    lastError_ = error;
    return false;
}
bool ActorComponentWorld::BeginActorPlay(ActorSlot& actor) {
    if (actor.begunPlay) return true;
    for (ComponentSlot& slot : actor.components) {
        if (slot.component == nullptr || slot.begunPlay) continue;
        dispatching_ = true;
        const bool began = slot.component->OnBeginPlay(sceneWorld_, actor.scene);
        dispatching_ = false;
        if (!began) {
            for (ComponentSlot& rollback : actor.components) if (rollback.component != nullptr && rollback.begunPlay) {
                rollback.component->OnEndPlay(sceneWorld_, actor.scene);
                rollback.begunPlay = false;
            }
            actor.begunPlay = false;
            return Fail(ActorComponentError::BeginPlayRejected);
        }
        slot.begunPlay = true;
    }
    actor.begunPlay = true;
    uint8_t activatedCount = 0U;
    for (uint8_t componentIndex = 0U; componentIndex < kMaxComponentsPerActor; ++componentIndex) {
        ComponentSlot& slot = actor.components[componentIndex];
        if (slot.component == nullptr || !slot.active) continue;
        dispatching_ = true;
        const bool activated = slot.component->OnActivate(sceneWorld_, actor.scene);
        dispatching_ = false;
        if (!activated) {
            for (int rollbackIndex = static_cast<int>(componentIndex) - 1; rollbackIndex >= 0; --rollbackIndex) {
                ComponentSlot& rollback = actor.components[static_cast<uint8_t>(rollbackIndex)];
                if (rollback.component == nullptr || !rollback.active) continue;
                dispatching_ = true;
                rollback.component->OnDeactivate(sceneWorld_, actor.scene);
                dispatching_ = false;
                --activatedCount;
            }
            for (ComponentSlot& rollback : actor.components) if (rollback.component != nullptr && rollback.begunPlay) {
                dispatching_ = true;
                rollback.component->OnEndPlay(sceneWorld_, actor.scene);
                dispatching_ = false;
                rollback.begunPlay = false;
            }
            actor.begunPlay = false;
            return Fail(ActorComponentError::ActivationRejected);
        }
        ++activatedCount;
    }
    return true;
}
bool ActorComponentWorld::EndActorPlay(ActorSlot& actor) {
    if (!actor.begunPlay) return true;
    for (ComponentSlot& slot : actor.components) {
        if (slot.component == nullptr || !slot.begunPlay) continue;
        dispatching_ = true;
        const bool ended = slot.component->OnEndPlay(sceneWorld_, actor.scene);
        dispatching_ = false;
        if (!ended) return Fail(ActorComponentError::EndPlayRejected);
        slot.begunPlay = false;
    }
    actor.begunPlay = false;
    return true;
}
bool ActorComponentWorld::ValidActor(SceneEntity actor) const {
    return actor.index < kCapacity && actors_[actor.index].registered && actors_[actor.index].scene == actor && sceneWorld_.GetTransform(actor) != nullptr;
}
ActorComponentWorld::ActorSlot* ActorComponentWorld::FindActorSlot(SceneEntity actor) {
    return ValidActor(actor) ? &actors_[actor.index] : nullptr;
}
const ActorComponentWorld::ActorSlot* ActorComponentWorld::FindActorSlot(SceneEntity actor) const {
    return ValidActor(actor) ? &actors_[actor.index] : nullptr;
}
ActorComponentWorld::ComponentSlot* ActorComponentWorld::FindSlot(SceneEntity actor, uint16_t typeId) {
    ActorSlot* slot = FindActorSlot(actor);
    if (slot == nullptr || typeId == 0U) return nullptr;
    for (ComponentSlot& component : slot->components) if (component.component != nullptr && component.component->TypeId() == typeId) return &component;
    return nullptr;
}
const ActorComponentWorld::ComponentSlot* ActorComponentWorld::FindSlot(SceneEntity actor, uint16_t typeId) const {
    const ActorSlot* slot = FindActorSlot(actor);
    if (slot == nullptr || typeId == 0U) return nullptr;
    for (const ComponentSlot& component : slot->components) if (component.component != nullptr && component.component->TypeId() == typeId) return &component;
    return nullptr;
}
bool ActorComponentWorld::CreateActor(SceneEntity& output, std::string name) {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    if (name.size() > kMaxNameBytes || name.find('\0') != std::string::npos) return Fail(ActorComponentError::InvalidName);
    if (actorCount_ >= kCapacity || registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    SceneEntity candidate{};
    if (!sceneWorld_.Create(candidate)) return Fail(ActorComponentError::Capacity);
    if (actors_[candidate.index].registered) {
        sceneWorld_.Destroy(candidate);
        return Fail(ActorComponentError::InvalidActor);
    }
    ActorSlot actor{};
    actor.registered = true;
    actor.scene = candidate;
    actor.name = std::move(name);
    actors_[candidate.index] = std::move(actor);
    ++actorCount_;
    ++registrationRevision_;
    if (begunPlay_ && !BeginActorPlay(actors_[candidate.index])) {
        actors_[candidate.index] = {};
        --actorCount_;
        --registrationRevision_;
        sceneWorld_.Destroy(candidate);
        return Fail(ActorComponentError::BeginPlayRejected);
    }
    output = candidate;
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::DestroyActor(SceneEntity actor) {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    ActorSlot* slot = FindActorSlot(actor);
    if (slot == nullptr) return Fail(ActorComponentError::InvalidActor);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    const uint8_t detachedCount = ComponentCount(actor);
    if (!EndActorPlay(*slot)) return false;
    for (const ComponentSlot& component : slot->components) {
        if (component.component == nullptr) continue;
        dispatching_ = true;
        const bool detached = component.component->OnDetach(sceneWorld_, actor);
        dispatching_ = false;
        if (!detached) return Fail(ActorComponentError::DetachRejected);
    }
    if (!sceneWorld_.Destroy(actor)) return Fail(ActorComponentError::InvalidActor);
    componentCount_ -= detachedCount;
    --actorCount_;
    ++registrationRevision_;
    actors_[actor.index] = {};
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::AttachComponent(SceneEntity actor, std::unique_ptr<IActorComponent> component) {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    if (!ValidActor(actor)) return Fail(ActorComponentError::InvalidActor);
    if (component == nullptr || component->TypeId() == 0U) return Fail(ActorComponentError::InvalidComponent);
    const std::string_view typeName = component->TypeName();
    if (typeName.size() > kMaxComponentTypeNameBytes || typeName.find('\0') != std::string_view::npos) return Fail(ActorComponentError::InvalidComponent);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    ActorSlot* slot = &actors_[actor.index];
    for (const ComponentSlot& existing : slot->components) if (existing.component != nullptr && existing.component->TypeId() == component->TypeId()) return Fail(ActorComponentError::DuplicateComponent);
    ComponentSlot* target = nullptr;
    for (ComponentSlot& existing : slot->components) if (existing.component == nullptr) { target = &existing; break; }
    if (target == nullptr) return Fail(ActorComponentError::Capacity);
    dispatching_ = true;
    const bool attached = component->OnAttach(sceneWorld_, actor);
    dispatching_ = false;
    if (!attached) return Fail(ActorComponentError::AttachRejected);
    if (slot->begunPlay) {
        dispatching_ = true;
        const bool began = component->OnBeginPlay(sceneWorld_, actor);
        dispatching_ = false;
        if (!began) {
            dispatching_ = true;
            component->OnDetach(sceneWorld_, actor);
            dispatching_ = false;
            return Fail(ActorComponentError::BeginPlayRejected);
        }
        dispatching_ = true;
        const bool activated = component->OnActivate(sceneWorld_, actor);
        dispatching_ = false;
        if (!activated) {
            dispatching_ = true;
            component->OnEndPlay(sceneWorld_, actor);
            component->OnDetach(sceneWorld_, actor);
            dispatching_ = false;
            return Fail(ActorComponentError::ActivationRejected);
        }
    }
    target->component = std::move(component);
    target->enabled = true;
    target->active = true;
    target->begunPlay = slot->begunPlay;
    ++componentCount_;
    ++registrationRevision_;
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::DetachComponent(SceneEntity actor, uint16_t typeId) {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    ComponentSlot* slot = FindSlot(actor, typeId);
    if (slot == nullptr) return Fail(ActorComponentError::InvalidComponent);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    ActorSlot* actorSlot = FindActorSlot(actor);
    if (actorSlot == nullptr) return Fail(ActorComponentError::InvalidActor);
    if (slot->begunPlay) {
        dispatching_ = true;
        const bool ended = slot->component->OnEndPlay(sceneWorld_, actor);
        dispatching_ = false;
        if (!ended) return Fail(ActorComponentError::EndPlayRejected);
        slot->begunPlay = false;
    }
    dispatching_ = true;
    const bool detached = slot->component->OnDetach(sceneWorld_, actor);
    dispatching_ = false;
    if (!detached) return Fail(ActorComponentError::DetachRejected);
    slot->component.reset();
    slot->enabled = true;
    slot->active = true;
    --componentCount_;
    ++registrationRevision_;
    if (actorSlot->begunPlay) {
        bool allBegun = true;
        for (const ComponentSlot& component : actorSlot->components) if (component.component != nullptr && !component.begunPlay) allBegun = false;
        actorSlot->begunPlay = allBegun;
    }
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::SetComponentEnabled(SceneEntity actor, uint16_t typeId, bool enabled) {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    ComponentSlot* slot = FindSlot(actor, typeId);
    if (slot == nullptr) return Fail(ActorComponentError::InvalidComponent);
    if (slot->enabled == enabled) { lastError_ = ActorComponentError::None; return true; }
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    slot->enabled = enabled;
    ++registrationRevision_;
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::SetComponentActive(SceneEntity actor, uint16_t typeId, bool active) {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    ComponentSlot* slot = FindSlot(actor, typeId);
    if (slot == nullptr) return Fail(ActorComponentError::InvalidComponent);
    if (slot->active == active) { lastError_ = ActorComponentError::None; return true; }
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    if (slot->begunPlay) {
        dispatching_ = true;
        const bool callback = active ? slot->component->OnActivate(sceneWorld_, actor) : slot->component->OnDeactivate(sceneWorld_, actor);
        dispatching_ = false;
        if (!callback) return Fail(active ? ActorComponentError::ActivationRejected : ActorComponentError::DeactivationRejected);
    }
    slot->active = active;
    ++registrationRevision_;
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::BeginPlay() {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    if (begunPlay_) { lastError_ = ActorComponentError::None; return true; }
    for (ActorSlot& actor : actors_) if (actor.registered && !BeginActorPlay(actor)) {
        for (ActorSlot& rollback : actors_) if (rollback.registered && rollback.begunPlay) EndActorPlay(rollback);
        begunPlay_ = false;
        return false;
    }
    begunPlay_ = true;
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::EndPlay() {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    if (!begunPlay_) { lastError_ = ActorComponentError::None; return true; }
    for (ActorSlot& actor : actors_) if (actor.registered && !EndActorPlay(actor)) return false;
    begunPlay_ = false;
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::TickFixed(uint32_t fixedTicks, ActorComponentWorldReceipt& receipt) {
    if (dispatching_) return Fail(ActorComponentError::MutationDuringDispatch);
    if (fixedTicks == 0U) return Fail(ActorComponentError::TickRejected);
    if (!begunPlay_ && !BeginPlay()) return false;
    for (const ActorSlot& actor : actors_) if (actor.registered) for (const ComponentSlot& slot : actor.components) if (slot.component != nullptr && slot.enabled && slot.active) {
        const uint8_t group = slot.component->TickGroup();
        const uint8_t order = slot.component->TickOrder();
        const uint8_t dependencyCount = slot.component->TickDependencyCount();
        if (group >= kMaxTickGroups || order >= kMaxTickOrders || dependencyCount > kMaxComponentsPerActor) return Fail(ActorComponentError::TickRejected);
        for (uint8_t dependencyIndex = 0U; dependencyIndex < dependencyCount; ++dependencyIndex) {
            const uint16_t dependencyType = slot.component->TickDependencyTypeId(dependencyIndex);
            const ComponentSlot* dependency = FindSlot(actor.scene, dependencyType);
            if (dependencyType == 0U || dependencyType == slot.component->TypeId() || dependency == nullptr || dependency->component == nullptr || !dependency->enabled || dependency->component->TickGroup() > group || (dependency->component->TickGroup() == group && dependency->component->TickOrder() >= order)) return Fail(ActorComponentError::DependencyRejected);
            for (uint8_t priorIndex = 0U; priorIndex < dependencyIndex; ++priorIndex) if (slot.component->TickDependencyTypeId(priorIndex) == dependencyType) return Fail(ActorComponentError::DependencyRejected);
        }
    }
    uint32_t ticked = 0U;
    for (uint8_t group = 0U; group < kMaxTickGroups; ++group) for (uint8_t order = 0U; order < kMaxTickOrders; ++order) for (uint16_t actorIndex = 0U; actorIndex < kCapacity; ++actorIndex) {
        ActorSlot& actor = actors_[actorIndex];
        if (!actor.registered) continue;
        for (ComponentSlot& slot : actor.components) {
            if (slot.component == nullptr || !slot.enabled || slot.component->TickGroup() != group || slot.component->TickOrder() != order) continue;
            dispatching_ = true;
            const bool tickedSuccessfully = slot.component->OnFixedTick(sceneWorld_, actor.scene, fixedTicks);
            dispatching_ = false;
            if (!tickedSuccessfully) {
                lastReceipt_ = {actorCount_, componentCount_, ticked, registrationRevision_};
                return Fail(ActorComponentError::TickRejected);
            }
            ++ticked;
        }
    }
    receipt = {actorCount_, componentCount_, ticked, registrationRevision_};
    lastReceipt_ = receipt;
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::CaptureSnapshot(ActorComponentWorldSnapshot& snapshot) const {
    ActorComponentWorldSnapshot candidate{};
    candidate.begunPlay = begunPlay_;
    candidate.registrationRevision = registrationRevision_;
    try {
        candidate.actors.reserve(actorCount_);
        candidate.componentBytes.reserve(kMaxActorComponentWorldSnapshotBytes < actorCount_ * kMaxComponentsPerActor ? kMaxActorComponentWorldSnapshotBytes : 0U);
        for (const ActorSlot& actor : actors_) {
            if (!actor.registered) continue;
            ActorComponentSnapshot record{};
            record.actor = actor.scene;
            record.name = actor.name;
            record.begunPlay = actor.begunPlay;
            for (const ComponentSlot& component : actor.components) if (component.component != nullptr) {
                if (record.componentCount >= kMaxComponentsPerActor) return Fail(ActorComponentError::SnapshotRejected);
                const uint16_t snapshotSize = component.component->SnapshotSizeBytes();
                if (snapshotSize > kMaxActorComponentSnapshotBytes || candidate.componentBytes.size() > kMaxActorComponentWorldSnapshotBytes - snapshotSize) return Fail(ActorComponentError::SnapshotRejected);
                const uint32_t offset = static_cast<uint32_t>(candidate.componentBytes.size());
                const std::string_view typeName = component.component->TypeName();
                if (typeName.size() > kMaxComponentTypeNameBytes || typeName.find('\0') != std::string_view::npos) return Fail(ActorComponentError::SnapshotRejected);
                record.componentTypeIds[record.componentCount] = component.component->TypeId();
                record.componentTypeNames[record.componentCount] = typeName;
                record.componentEnabled[record.componentCount] = component.enabled;
                record.componentActive[record.componentCount] = component.active;
                record.snapshotOffsets[record.componentCount] = offset;
                record.snapshotSizes[record.componentCount] = snapshotSize;
                if (snapshotSize != 0U) {
                    const size_t oldSize = candidate.componentBytes.size();
                    candidate.componentBytes.resize(oldSize + snapshotSize);
                    dispatching_ = true;
                    const bool captured = component.component->CaptureSnapshot(std::span<uint8_t>(candidate.componentBytes.data() + oldSize, snapshotSize));
                    dispatching_ = false;
                    if (!captured) return Fail(ActorComponentError::SnapshotRejected);
                } else {
                    dispatching_ = true;
                    const bool captured = component.component->CaptureSnapshot({});
                    dispatching_ = false;
                    if (!captured) return Fail(ActorComponentError::SnapshotRejected);
                }
                ++record.componentCount;
            }
            candidate.actors.push_back(std::move(record));
        }
    } catch (const std::bad_alloc&) {
        return Fail(ActorComponentError::SnapshotRejected);
    }
    snapshot = std::move(candidate);
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::RestoreSnapshot(const ActorComponentWorldSnapshot& snapshot) {
    if (dispatching_ || snapshot.actors.size() != actorCount_ || snapshot.begunPlay != begunPlay_ || snapshot.componentBytes.size() > kMaxActorComponentWorldSnapshotBytes) return Fail(dispatching_ ? ActorComponentError::MutationDuringDispatch : ActorComponentError::RestoreRejected);
    uint32_t expectedComponentCount = 0U;
    for (const ActorComponentSnapshot& record : snapshot.actors) {
        const ActorSlot* actor = FindActorSlot(record.actor);
        if (actor == nullptr || record.name != actor->name || record.componentCount > kMaxComponentsPerActor || record.begunPlay != actor->begunPlay) return Fail(ActorComponentError::RestoreRejected);
        expectedComponentCount += record.componentCount;
        if (expectedComponentCount > componentCount_) return Fail(ActorComponentError::RestoreRejected);
        for (uint8_t componentIndex = 0U; componentIndex < record.componentCount; ++componentIndex) {
            const uint16_t typeId = record.componentTypeIds[componentIndex];
            const ComponentSlot* component = FindSlot(record.actor, typeId);
            const uint32_t offset = record.snapshotOffsets[componentIndex];
            const uint16_t size = record.snapshotSizes[componentIndex];
            const std::string_view typeName = component == nullptr || component->component == nullptr ? std::string_view{} : component->component->TypeName();
            if (component == nullptr || component->component == nullptr || typeName.size() > kMaxComponentTypeNameBytes || typeName.find('\0') != std::string_view::npos || record.componentTypeNames[componentIndex] != typeName || component->component->TypeId() != typeId || offset > snapshot.componentBytes.size() || snapshot.componentBytes.size() - offset < size || size > kMaxActorComponentSnapshotBytes || component->component->SnapshotSizeBytes() != size) return Fail(ActorComponentError::RestoreRejected);
            for (uint8_t priorIndex = 0U; priorIndex < componentIndex; ++priorIndex) if (record.componentTypeIds[priorIndex] == typeId) return Fail(ActorComponentError::RestoreRejected);
            dispatching_ = true;
            const bool valid = component->component->ValidateSnapshot(std::span<const uint8_t>(snapshot.componentBytes.data() + offset, size));
            dispatching_ = false;
            if (!valid) return Fail(ActorComponentError::RestoreRejected);
        }
    }
    if (expectedComponentCount != componentCount_) return Fail(ActorComponentError::RestoreRejected);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    ActorComponentWorldSnapshot backup{};
    if (!CaptureSnapshot(backup)) return Fail(ActorComponentError::RestoreRejected);
    if (!RestoreValidated(snapshot)) {
        RestoreValidated(backup);
        return Fail(ActorComponentError::RestoreRejected);
    }
    ++registrationRevision_;
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::RestoreValidated(const ActorComponentWorldSnapshot& snapshot) {
    for (const ActorComponentSnapshot& record : snapshot.actors) for (uint8_t componentIndex = 0U; componentIndex < record.componentCount; ++componentIndex) {
        ComponentSlot* component = FindSlot(record.actor, record.componentTypeIds[componentIndex]);
        const uint32_t offset = record.snapshotOffsets[componentIndex]; const uint16_t size = record.snapshotSizes[componentIndex];
        dispatching_ = true;
        const bool restored = component != nullptr && component->component != nullptr && component->component->RestoreSnapshot(std::span<const uint8_t>(snapshot.componentBytes.data() + offset, size));
        dispatching_ = false;
        if (!restored) return false;
        component->enabled = record.componentEnabled[componentIndex]; component->active = record.componentActive[componentIndex];
    }
    return true;
}
bool ActorComponentWorld::CollectActors(std::vector<SceneEntity>& output) const {
    std::vector<SceneEntity> candidate;
    try {
        candidate.reserve(actorCount_);
        for (const ActorSlot& actor : actors_) if (actor.registered) candidate.push_back(actor.scene);
    } catch (const std::bad_alloc&) {
        return Fail(ActorComponentError::SnapshotRejected);
    }
    output = std::move(candidate);
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::CollectComponentTypes(SceneEntity actor, std::vector<uint16_t>& output) const {
    const ActorSlot* slot = FindActorSlot(actor);
    if (slot == nullptr) return Fail(ActorComponentError::InvalidActor);
    std::vector<uint16_t> candidate;
    try {
        candidate.reserve(kMaxComponentsPerActor);
        for (const ComponentSlot& component : slot->components) if (component.component != nullptr) candidate.push_back(component.component->TypeId());
    } catch (const std::bad_alloc&) {
        return Fail(ActorComponentError::SnapshotRejected);
    }
    output = std::move(candidate);
    lastError_ = ActorComponentError::None;
    return true;
}
bool ActorComponentWorld::IsActorAlive(SceneEntity actor) const { return ValidActor(actor); }
const std::string* ActorComponentWorld::ActorName(SceneEntity actor) const {
    const ActorSlot* slot = FindActorSlot(actor);
    return slot == nullptr ? nullptr : &slot->name;
}
IActorComponent* ActorComponentWorld::FindComponent(SceneEntity actor, uint16_t typeId) {
    ComponentSlot* slot = FindSlot(actor, typeId);
    return slot == nullptr ? nullptr : slot->component.get();
}
const IActorComponent* ActorComponentWorld::FindComponent(SceneEntity actor, uint16_t typeId) const {
    const ComponentSlot* slot = FindSlot(actor, typeId);
    return slot == nullptr ? nullptr : slot->component.get();
}
bool ActorComponentWorld::IsComponentEnabled(SceneEntity actor, uint16_t typeId) const {
    const ComponentSlot* slot = FindSlot(actor, typeId);
    return slot != nullptr && slot->enabled;
}
bool ActorComponentWorld::IsComponentActive(SceneEntity actor, uint16_t typeId) const {
    const ComponentSlot* slot = FindSlot(actor, typeId);
    return slot != nullptr && slot->active;
}
uint8_t ActorComponentWorld::ComponentCount(SceneEntity actor) const {
    const ActorSlot* slot = FindActorSlot(actor);
    if (slot == nullptr) return 0U;
    uint8_t count = 0U;
    for (const ComponentSlot& component : slot->components) if (component.component != nullptr) ++count;
    return count;
}
} // namespace NeoEngine
