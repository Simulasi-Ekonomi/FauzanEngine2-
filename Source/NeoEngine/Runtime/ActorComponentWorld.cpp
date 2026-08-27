#include "ActorComponentWorld.h"
#include <limits>
#include <new>
namespace NeoEngine {
namespace {
template <typename Callback> bool InvokeDispatch(bool& dispatching, Callback callback) { dispatching = true; try { const bool result = callback(); dispatching = false; return result; } catch (...) { dispatching = false; return false; } }
struct TickMetadata { uint8_t group = 0U; uint8_t order = 0U; uint8_t dependencyCount = 0U; uint16_t typeId = 0U; };
struct SnapshotMetadata { uint16_t size = 0U; uint16_t typeId = 0U; std::string_view typeName{}; };
bool ReadComponentTypeId(bool& dispatching, const IActorComponent& component, uint16_t& typeId) { return InvokeDispatch(dispatching, [&] { typeId = component.TypeId(); return true; }); }
bool ReadComponentTypeAndName(bool& dispatching, const IActorComponent& component, uint16_t& typeId, std::string_view& typeName) { return InvokeDispatch(dispatching, [&] { typeId = component.TypeId(); typeName = component.TypeName(); return true; }); }
bool ReadSnapshotMetadata(bool& dispatching, const IActorComponent& component, SnapshotMetadata& metadata) { return InvokeDispatch(dispatching, [&] { metadata = {component.SnapshotSizeBytes(), component.TypeId(), component.TypeName()}; return true; }); }
bool ReadTickMetadata(bool& dispatching, IActorComponent& component, TickMetadata& metadata) { return InvokeDispatch(dispatching, [&] { metadata = {component.TickGroup(), component.TickOrder(), component.TickDependencyCount(), component.TypeId()}; return true; }); }
bool ReadDependencyType(bool& dispatching, IActorComponent& component, uint8_t index, uint16_t& typeId) { return InvokeDispatch(dispatching, [&] { typeId = component.TickDependencyTypeId(index); return true; }); }
}
ActorComponentWorld::ActorComponentWorld(SceneWorld& sceneWorld) : sceneWorld_(sceneWorld) {}
ActorComponentWorld::~ActorComponentWorld() {
    for (uint16_t index = 0U; index < kCapacity; ++index) {
        if (!actors_[index].registered) continue;
        ActorSlot& actor = actors_[index];
        if (actor.begunPlay) EndActorPlay(actor);
        for (uint8_t componentIndex = 0U; componentIndex < kMaxComponentsPerActor; ++componentIndex) {
            ComponentSlot& slot = actor.components[componentIndex];
            if (slot.component != nullptr) (void)InvokeDispatch(dispatching_, [&] { return slot.component->OnDetach(sceneWorld_, actor.scene); });
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
        const bool began = InvokeDispatch(dispatching_, [&] { return slot.component->OnBeginPlay(sceneWorld_, actor.scene); });
        if (!began) {
            bool rollbackRejected = false;
            for (ComponentSlot& rollback : actor.components) if (rollback.component != nullptr && rollback.begunPlay) {
                const bool ended = InvokeDispatch(dispatching_, [&] { return rollback.component->OnEndPlay(sceneWorld_, actor.scene); });
                if (ended) rollback.begunPlay = false; else rollbackRejected = true;
            }
            actor.begunPlay = false;
            return Fail(rollbackRejected ? ActorComponentError::RollbackRejected : ActorComponentError::BeginPlayRejected);
        }
        slot.begunPlay = true;
    }
    actor.begunPlay = true;
    uint8_t activatedCount = 0U;
    for (uint8_t componentIndex = 0U; componentIndex < kMaxComponentsPerActor; ++componentIndex) {
        ComponentSlot& slot = actor.components[componentIndex];
        if (slot.component == nullptr || !slot.active) continue;
        const bool activated = InvokeDispatch(dispatching_, [&] { return slot.component->OnActivate(sceneWorld_, actor.scene); });
        if (!activated) {
            bool rollbackRejected = false;
            for (int rollbackIndex = static_cast<int>(componentIndex) - 1; rollbackIndex >= 0; --rollbackIndex) {
                ComponentSlot& rollback = actor.components[static_cast<uint8_t>(rollbackIndex)];
                if (rollback.component == nullptr || !rollback.active) continue;
                const bool deactivated = InvokeDispatch(dispatching_, [&] { return rollback.component->OnDeactivate(sceneWorld_, actor.scene); });
                if (!deactivated) rollbackRejected = true;
                if (activatedCount > 0U) --activatedCount;
            }
            for (ComponentSlot& rollback : actor.components) if (rollback.component != nullptr && rollback.begunPlay) {
                const bool ended = InvokeDispatch(dispatching_, [&] { return rollback.component->OnEndPlay(sceneWorld_, actor.scene); });
                if (ended) rollback.begunPlay = false; else rollbackRejected = true;
            }
            actor.begunPlay = false;
            return Fail(rollbackRejected ? ActorComponentError::RollbackRejected : ActorComponentError::ActivationRejected);
        }
        ++activatedCount;
    }
    return true;
}
bool ActorComponentWorld::EndActorPlay(ActorSlot& actor) {
    bool hasBegunComponent = false;
    for (const ComponentSlot& slot : actor.components) if (slot.component != nullptr && slot.begunPlay) { hasBegunComponent = true; break; }
    if (!hasBegunComponent) { actor.begunPlay = false; return true; }
    for (ComponentSlot& slot : actor.components) {
        if (slot.component == nullptr || !slot.begunPlay) continue;
        const bool ended = InvokeDispatch(dispatching_, [&] { return slot.component->OnEndPlay(sceneWorld_, actor.scene); });
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
    for (ComponentSlot& component : slot->components) {
        uint16_t existingTypeId = 0U;
        if (component.component != nullptr && ReadComponentTypeId(dispatching_, *component.component, existingTypeId) && existingTypeId == typeId) return &component;
    }
    return nullptr;
}
const ActorComponentWorld::ComponentSlot* ActorComponentWorld::FindSlot(SceneEntity actor, uint16_t typeId) const {
    const ActorSlot* slot = FindActorSlot(actor);
    if (slot == nullptr || typeId == 0U) return nullptr;
    for (const ComponentSlot& component : slot->components) {
        uint16_t existingTypeId = 0U;
        if (component.component != nullptr && ReadComponentTypeId(dispatching_, *component.component, existingTypeId) && existingTypeId == typeId) return &component;
    }
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
        const bool detached = InvokeDispatch(dispatching_, [&] { return component.component->OnDetach(sceneWorld_, actor); });
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
    if (component == nullptr) return Fail(ActorComponentError::InvalidComponent);
    uint16_t componentTypeId = 0U; std::string_view typeName{};
    if (!ReadComponentTypeAndName(dispatching_, *component, componentTypeId, typeName) || componentTypeId == 0U) return Fail(ActorComponentError::InvalidComponent);
    if (typeName.size() > kMaxComponentTypeNameBytes || typeName.find('\0') != std::string_view::npos) return Fail(ActorComponentError::InvalidComponent);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    ActorSlot* slot = &actors_[actor.index];
    ComponentSlot* target = nullptr;
    for (const ComponentSlot& existing : slot->components) if (existing.component != nullptr) {
        uint16_t existingTypeId = 0U;
        if (!ReadComponentTypeId(dispatching_, *existing.component, existingTypeId)) return Fail(ActorComponentError::InvalidComponent);
        if (existingTypeId == componentTypeId) return Fail(ActorComponentError::DuplicateComponent);
    }
    for (ComponentSlot& existing : slot->components) if (existing.component == nullptr) { target = &existing; break; }
    if (target == nullptr) return Fail(ActorComponentError::Capacity);
    const bool attached = InvokeDispatch(dispatching_, [&] { return component->OnAttach(sceneWorld_, actor); });
    if (!attached) return Fail(ActorComponentError::AttachRejected);
    if (slot->begunPlay) {
        const bool began = InvokeDispatch(dispatching_, [&] { return component->OnBeginPlay(sceneWorld_, actor); });
        if (!began) {
            const bool detached = InvokeDispatch(dispatching_, [&] { return component->OnDetach(sceneWorld_, actor); });
            return Fail(detached ? ActorComponentError::BeginPlayRejected : ActorComponentError::RollbackRejected);
        }
        const bool activated = InvokeDispatch(dispatching_, [&] { return component->OnActivate(sceneWorld_, actor); });
        if (!activated) {
            const bool ended = InvokeDispatch(dispatching_, [&] { return component->OnEndPlay(sceneWorld_, actor); });
            const bool detached = InvokeDispatch(dispatching_, [&] { return component->OnDetach(sceneWorld_, actor); });
            return Fail(ended && detached ? ActorComponentError::ActivationRejected : ActorComponentError::RollbackRejected);
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
        const bool ended = InvokeDispatch(dispatching_, [&] { return slot->component->OnEndPlay(sceneWorld_, actor); });
        if (!ended) return Fail(ActorComponentError::EndPlayRejected);
        slot->begunPlay = false;
    }
    const bool detached = InvokeDispatch(dispatching_, [&] { return slot->component->OnDetach(sceneWorld_, actor); });
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
        const bool callback = InvokeDispatch(dispatching_, [&] { return active ? slot->component->OnActivate(sceneWorld_, actor) : slot->component->OnDeactivate(sceneWorld_, actor); });
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
        const ActorComponentError failureError = lastError_;
        bool rollbackRejected = false;
        for (ActorSlot& rollback : actors_) {
            bool hasBegunComponent = false;
            for (const ComponentSlot& slot : rollback.components) if (slot.component != nullptr && slot.begunPlay) { hasBegunComponent = true; break; }
            if (rollback.registered && (rollback.begunPlay || hasBegunComponent) && !EndActorPlay(rollback)) rollbackRejected = true;
        }
        begunPlay_ = false;
        return Fail(rollbackRejected ? ActorComponentError::RollbackRejected : failureError);
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
    if (fixedTicks == 0U || fixedTicks > kMaxFixedTicks) return Fail(ActorComponentError::TickRejected);
    if (!begunPlay_ && !BeginPlay()) return false;
    struct StagedTick { IActorComponent* component = nullptr; SceneEntity actor{}; uint8_t group = 0U; uint8_t order = 0U; };
    std::unique_ptr<StagedTick[]> staged;
    try { staged = std::make_unique<StagedTick[]>(static_cast<size_t>(kCapacity) * kMaxComponentsPerActor); }
    catch (const std::bad_alloc&) { return Fail(ActorComponentError::Capacity); }
    uint16_t stagedCount = 0U;
    for (const ActorSlot& actor : actors_) if (actor.registered) for (const ComponentSlot& slot : actor.components) if (slot.component != nullptr && slot.enabled && slot.active) {
        TickMetadata metadata{};
        if (stagedCount >= static_cast<size_t>(kCapacity) * kMaxComponentsPerActor || !ReadTickMetadata(dispatching_, *slot.component, metadata) || metadata.group >= kMaxTickGroups || metadata.order >= kMaxTickOrders || metadata.dependencyCount > kMaxComponentsPerActor) return Fail(ActorComponentError::TickRejected);
        for (uint8_t dependencyIndex = 0U; dependencyIndex < metadata.dependencyCount; ++dependencyIndex) {
            uint16_t dependencyType = 0U;
            if (!ReadDependencyType(dispatching_, *slot.component, dependencyIndex, dependencyType)) return Fail(ActorComponentError::TickRejected);
            const ComponentSlot* dependency = FindSlot(actor.scene, dependencyType);
            if (dependencyType == 0U || dependencyType == metadata.typeId || dependency == nullptr || dependency->component == nullptr || !dependency->enabled || !dependency->active) return Fail(ActorComponentError::DependencyRejected);
            TickMetadata dependencyMetadata{};
            if (!ReadTickMetadata(dispatching_, *dependency->component, dependencyMetadata)) return Fail(ActorComponentError::TickRejected);
            if (dependencyMetadata.group > metadata.group || (dependencyMetadata.group == metadata.group && dependencyMetadata.order >= metadata.order)) return Fail(ActorComponentError::DependencyRejected);
            for (uint8_t priorIndex = 0U; priorIndex < dependencyIndex; ++priorIndex) { uint16_t priorType = 0U; if (!ReadDependencyType(dispatching_, *slot.component, priorIndex, priorType)) return Fail(ActorComponentError::TickRejected); if (priorType == dependencyType) return Fail(ActorComponentError::DependencyRejected); }
        }
        staged[stagedCount++] = {slot.component.get(), actor.scene, metadata.group, metadata.order};
    }
    uint32_t ticked = 0U;
    for (uint8_t group = 0U; group < kMaxTickGroups; ++group) for (uint8_t order = 0U; order < kMaxTickOrders; ++order) for (uint16_t index = 0U; index < stagedCount; ++index) {
        StagedTick& entry = staged[index];
        if (entry.group != group || entry.order != order) continue;
        const bool tickedSuccessfully = InvokeDispatch(dispatching_, [&] { return entry.component->OnFixedTick(sceneWorld_, entry.actor, fixedTicks); });
        if (!tickedSuccessfully) {
            lastReceipt_ = {actorCount_, componentCount_, ticked, registrationRevision_};
            return Fail(ActorComponentError::TickRejected);
        }
        ++ticked;
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
                SnapshotMetadata metadata{};
                if (!ReadSnapshotMetadata(dispatching_, *component.component, metadata)) return Fail(ActorComponentError::SnapshotRejected);
                const uint16_t snapshotSize = metadata.size;
                if (snapshotSize > kMaxActorComponentSnapshotBytes || candidate.componentBytes.size() > kMaxActorComponentWorldSnapshotBytes - snapshotSize) return Fail(ActorComponentError::SnapshotRejected);
                const uint32_t offset = static_cast<uint32_t>(candidate.componentBytes.size());
                const std::string_view typeName = metadata.typeName;
                if (typeName.size() > kMaxComponentTypeNameBytes || typeName.find('\0') != std::string_view::npos || metadata.typeId == 0U) return Fail(ActorComponentError::SnapshotRejected);
                record.componentTypeIds[record.componentCount] = metadata.typeId;
                record.componentTypeNames[record.componentCount] = typeName;
                record.componentEnabled[record.componentCount] = component.enabled;
                record.componentActive[record.componentCount] = component.active;
                record.snapshotOffsets[record.componentCount] = offset;
                record.snapshotSizes[record.componentCount] = snapshotSize;
                if (snapshotSize != 0U) {
                    const size_t oldSize = candidate.componentBytes.size();
                    candidate.componentBytes.resize(oldSize + snapshotSize);
                    const bool captured = InvokeDispatch(dispatching_, [&] { return component.component->CaptureSnapshot(std::span<uint8_t>(candidate.componentBytes.data() + oldSize, snapshotSize)); });
                    if (!captured) return Fail(ActorComponentError::SnapshotRejected);
                } else {
                    const bool captured = InvokeDispatch(dispatching_, [&] { return component.component->CaptureSnapshot({}); });
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
            SnapshotMetadata metadata{};
            if (component == nullptr || component->component == nullptr || !ReadSnapshotMetadata(dispatching_, *component->component, metadata)) return Fail(ActorComponentError::RestoreRejected);
            const std::string_view typeName = metadata.typeName;
            if (typeName.size() > kMaxComponentTypeNameBytes || typeName.find('\0') != std::string_view::npos || metadata.typeId != typeId || record.componentTypeNames[componentIndex] != typeName || offset > snapshot.componentBytes.size() || snapshot.componentBytes.size() - offset < size || size > kMaxActorComponentSnapshotBytes || metadata.size != size) return Fail(ActorComponentError::RestoreRejected);
            for (uint8_t priorIndex = 0U; priorIndex < componentIndex; ++priorIndex) if (record.componentTypeIds[priorIndex] == typeId) return Fail(ActorComponentError::RestoreRejected);
            const bool valid = InvokeDispatch(dispatching_, [&] { return component->component->ValidateSnapshot(std::span<const uint8_t>(snapshot.componentBytes.data() + offset, size)); });
            if (!valid) return Fail(ActorComponentError::RestoreRejected);
        }
    }
    if (expectedComponentCount != componentCount_) return Fail(ActorComponentError::RestoreRejected);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    ActorComponentWorldSnapshot backup{};
    if (!CaptureSnapshot(backup)) return Fail(ActorComponentError::RestoreRejected);
    if (!RestoreValidated(snapshot)) {
        const bool rolledBack = RestoreValidated(backup);
        return Fail(rolledBack ? ActorComponentError::RestoreRejected : ActorComponentError::RollbackRejected);
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
        const bool restored = InvokeDispatch(dispatching_, [&] { return component != nullptr && component->component != nullptr && component->component->RestoreSnapshot(std::span<const uint8_t>(snapshot.componentBytes.data() + offset, size)); });
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
        for (const ComponentSlot& component : slot->components) if (component.component != nullptr) {
        uint16_t typeId = 0U;
        if (!ReadComponentTypeId(dispatching_, *component.component, typeId) || typeId == 0U) return Fail(ActorComponentError::SnapshotRejected);
        candidate.push_back(typeId);
    }
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
