#include "ActorComponentWorld.h"

#include <limits>

namespace NeoEngine {

ActorComponentWorld::ActorComponentWorld(SceneWorld& sceneWorld) : sceneWorld_(sceneWorld) {}

ActorComponentWorld::~ActorComponentWorld() {
    for (uint16_t index = 0U; index < kCapacity; ++index) {
        if (!actors_[index].registered) continue;
        for (uint8_t componentIndex = 0U; componentIndex < kMaxComponentsPerActor; ++componentIndex) {
            ComponentSlot& slot = actors_[index].components[componentIndex];
            if (slot.component != nullptr) slot.component->OnDetach(sceneWorld_, actors_[index].scene);
        }
    }
}

bool ActorComponentWorld::Fail(ActorComponentError error) {
    lastError_ = error;
    return false;
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
    output = candidate;
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}

bool ActorComponentWorld::DestroyActor(SceneEntity actor) {
    ActorSlot* slot = FindActorSlot(actor);
    if (slot == nullptr) return Fail(ActorComponentError::InvalidActor);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    const uint8_t detachedCount = ComponentCount(actor);
    for (const ComponentSlot& component : slot->components) {
        if (component.component != nullptr && !component.component->OnDetach(sceneWorld_, actor)) return Fail(ActorComponentError::DetachRejected);
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
    if (!ValidActor(actor)) return Fail(ActorComponentError::InvalidActor);
    if (component == nullptr || component->TypeId() == 0U) return Fail(ActorComponentError::InvalidComponent);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    ActorSlot* slot = &actors_[actor.index];
    for (const ComponentSlot& existing : slot->components) if (existing.component != nullptr && existing.component->TypeId() == component->TypeId()) return Fail(ActorComponentError::DuplicateComponent);
    ComponentSlot* target = nullptr;
    for (ComponentSlot& existing : slot->components) if (existing.component == nullptr) { target = &existing; break; }
    if (target == nullptr) return Fail(ActorComponentError::Capacity);
    if (!component->OnAttach(sceneWorld_, actor)) return Fail(ActorComponentError::AttachRejected);
    target->component = std::move(component);
    target->enabled = true;
    ++componentCount_;
    ++registrationRevision_;
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}

bool ActorComponentWorld::DetachComponent(SceneEntity actor, uint16_t typeId) {
    ComponentSlot* slot = FindSlot(actor, typeId);
    if (slot == nullptr) return Fail(ActorComponentError::InvalidComponent);
    if (registrationRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(ActorComponentError::Capacity);
    if (!slot->component->OnDetach(sceneWorld_, actor)) return Fail(ActorComponentError::DetachRejected);
    slot->component.reset();
    slot->enabled = true;
    --componentCount_;
    ++registrationRevision_;
    lastReceipt_ = {actorCount_, componentCount_, 0U, registrationRevision_};
    lastError_ = ActorComponentError::None;
    return true;
}

bool ActorComponentWorld::SetComponentEnabled(SceneEntity actor, uint16_t typeId, bool enabled) {
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

bool ActorComponentWorld::TickFixed(uint32_t fixedTicks, ActorComponentWorldReceipt& receipt) {
    if (fixedTicks == 0U) return Fail(ActorComponentError::TickRejected);
    uint32_t ticked = 0U;
    for (uint16_t actorIndex = 0U; actorIndex < kCapacity; ++actorIndex) {
        ActorSlot& actor = actors_[actorIndex];
        if (!actor.registered) continue;
        for (ComponentSlot& slot : actor.components) {
            if (slot.component == nullptr || !slot.enabled) continue;
            if (!slot.component->OnFixedTick(sceneWorld_, actor.scene, fixedTicks)) {
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

uint8_t ActorComponentWorld::ComponentCount(SceneEntity actor) const {
    const ActorSlot* slot = FindActorSlot(actor);
    if (slot == nullptr) return 0U;
    uint8_t count = 0U;
    for (const ComponentSlot& component : slot->components) if (component.component != nullptr) ++count;
    return count;
}

} // namespace NeoEngine
