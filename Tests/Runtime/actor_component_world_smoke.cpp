#include "Runtime/ActorComponentWorld.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
class ProbeComponent : public NeoEngine::IActorComponent {
public:
    explicit ProbeComponent(uint16_t typeId, uint32_t* detachSink = nullptr, uint32_t* endPlaySink = nullptr) : typeId_(typeId), detachSink_(detachSink), endPlaySink_(endPlaySink) {}
    uint16_t TypeId() const override { return typeId_; }
    std::string_view TypeName() const override { return "ProbeComponent"; }
    bool OnAttach(NeoEngine::SceneWorld& world, NeoEngine::SceneEntity actor) override { ++attachCount; return world.GetTransform(actor) != nullptr; }
    bool OnDetach(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++detachCount; if (detachSink_ != nullptr) ++*detachSink_; return true; }
    bool OnBeginPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++beginPlayCount; return true; }
    bool OnEndPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++endPlayCount; if (endPlaySink_ != nullptr) ++*endPlaySink_; return true; }
    bool OnActivate(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++activateCount; return true; }
    bool OnDeactivate(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++deactivateCount; return true; }
    uint8_t TickGroup() const override { return typeId_ == 11U ? 1U : 0U; }
    uint8_t TickOrder() const override { return typeId_ == 10U ? 1U : 0U; }
    uint16_t SnapshotSizeBytes() const override { return typeId_ == 10U ? sizeof(uint32_t) : 0U; }
    bool CaptureSnapshot(std::span<uint8_t> bytes) const override { if (typeId_ != 10U) return bytes.empty(); if (bytes.size() != sizeof(uint32_t)) return false; if (snapshotWorld_ != nullptr && (snapshotWorld_->SetComponentEnabled(snapshotActor_, typeId_, false) || snapshotWorld_->LastError() != NeoEngine::ActorComponentError::MutationDuringDispatch)) return false; std::memcpy(bytes.data(), &snapshotValue, sizeof(snapshotValue)); return true; }
    bool ValidateSnapshot(std::span<const uint8_t> bytes) const override { return typeId_ != 10U ? bytes.empty() : bytes.size() == sizeof(uint32_t); }
    bool RestoreSnapshot(std::span<const uint8_t> bytes) override { if (typeId_ != 10U) return bytes.empty(); if (bytes.size() != sizeof(uint32_t)) return false; std::memcpy(&snapshotValue, bytes.data(), sizeof(snapshotValue)); return true; }
    uint32_t snapshotValue = 0U;
    NeoEngine::ActorComponentWorld* snapshotWorld_ = nullptr;
    NeoEngine::SceneEntity snapshotActor_{};
    bool OnFixedTick(NeoEngine::SceneWorld&, NeoEngine::SceneEntity, uint32_t fixedTicks) override { tickedFixedTicks += fixedTicks; ++tickCalls; return true; }
    uint16_t typeId_ = 0U;
    uint32_t* detachSink_ = nullptr;
    uint32_t* endPlaySink_ = nullptr;
    uint32_t attachCount = 0U;
    uint32_t detachCount = 0U;
    uint32_t tickCalls = 0U;
    uint32_t tickedFixedTicks = 0U;
    uint32_t beginPlayCount = 0U;
    uint32_t endPlayCount = 0U;
    uint32_t activateCount = 0U;
    uint32_t deactivateCount = 0U;
};

class RejectDetachComponent final : public ProbeComponent {
public:
    RejectDetachComponent() : ProbeComponent(20U) {}
    bool OnDetach(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { return false; }
};

class RejectActivationComponent final : public ProbeComponent {
public:
    RejectActivationComponent() : ProbeComponent(21U) {}
    bool OnActivate(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { return false; }
};

class RejectDeactivationComponent final : public ProbeComponent {
public:
    RejectDeactivationComponent() : ProbeComponent(22U) {}
    bool OnDeactivate(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { return false; }
};

class ThrowingTickComponent final : public ProbeComponent {
public:
    ThrowingTickComponent() : ProbeComponent(23U) {}
    bool OnFixedTick(NeoEngine::SceneWorld&, NeoEngine::SceneEntity, uint32_t) override { throw std::runtime_error("tick failure"); }
};

class ThrowingTickMetadataComponent final : public ProbeComponent {
public:
    ThrowingTickMetadataComponent() : ProbeComponent(24U) {}
    uint8_t TickGroup() const override { throw std::runtime_error("tick metadata failure"); }
};

class ThrowingSnapshotMetadataComponent final : public ProbeComponent {
public:
    ThrowingSnapshotMetadataComponent() : ProbeComponent(25U) {}
    uint16_t SnapshotSizeBytes() const override { throw std::runtime_error("snapshot metadata failure"); }
};

class ThrowingBeginPlayComponent final : public ProbeComponent {
public:
    ThrowingBeginPlayComponent() : ProbeComponent(43U) {}
    bool OnBeginPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { throw std::runtime_error("begin play failure"); }
};

class MissingDependencyComponent final : public ProbeComponent {
public:
    MissingDependencyComponent() : ProbeComponent(31U) {}
    uint8_t TickDependencyCount() const override { return 1U; }
    uint16_t TickDependencyTypeId(uint8_t) const override { return 999U; }
};

class FailOnceRestoreComponent final : public ProbeComponent {
public:
    FailOnceRestoreComponent() : ProbeComponent(40U) {}
    uint16_t SnapshotSizeBytes() const override { return sizeof(uint32_t); }
    bool CaptureSnapshot(std::span<uint8_t> bytes) const override { if (bytes.size() != sizeof(uint32_t)) return false; std::memcpy(bytes.data(), &snapshotValue, sizeof(snapshotValue)); return true; }
    bool ValidateSnapshot(std::span<const uint8_t> bytes) const override { return bytes.size() == sizeof(uint32_t); }
    bool RestoreSnapshot(std::span<const uint8_t> bytes) override { if (bytes.size() != sizeof(uint32_t)) return false; std::memcpy(&snapshotValue, bytes.data(), sizeof(snapshotValue)); if (failNextRestore) { failNextRestore = false; return false; } return true; }
    bool failNextRestore = true;
};

class FailAlwaysRestoreComponent final : public ProbeComponent {
public:
    FailAlwaysRestoreComponent() : ProbeComponent(41U) {}
    uint16_t SnapshotSizeBytes() const override { return sizeof(uint32_t); }
    bool CaptureSnapshot(std::span<uint8_t> bytes) const override { if (bytes.size() != sizeof(uint32_t)) return false; std::memcpy(bytes.data(), &snapshotValue, sizeof(snapshotValue)); return true; }
    bool ValidateSnapshot(std::span<const uint8_t> bytes) const override { return bytes.size() == sizeof(uint32_t); }
    bool RestoreSnapshot(std::span<const uint8_t> bytes) override { if (bytes.size() != sizeof(uint32_t)) return false; std::memcpy(&snapshotValue, bytes.data(), sizeof(snapshotValue)); return false; }
    uint32_t snapshotValue = 0U;
};

class RejectEndPlayOnceComponent final : public ProbeComponent {
public:
    RejectEndPlayOnceComponent() : ProbeComponent(42U) {}
    bool OnEndPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { if (rejectNext) { rejectNext = false; return false; } return true; }
    bool rejectNext = true;
};

class RejectBeginPlayOnceComponent final : public ProbeComponent {
public:
    RejectBeginPlayOnceComponent() : ProbeComponent(43U) {}
    bool OnBeginPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { if (rejectNext) { rejectNext = false; return false; } return true; }
    bool rejectNext = true;
};

class RejectAttachBeginPlayComponent final : public ProbeComponent {
public:
    RejectAttachBeginPlayComponent() : ProbeComponent(44U) {}
    bool OnBeginPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { return false; }
};

class RejectAttachDetachComponent final : public ProbeComponent {
public:
    RejectAttachDetachComponent() : ProbeComponent(45U) {}
    bool OnBeginPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { return false; }
    bool OnDetach(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { return false; }
};

class ReentrantMutationComponent final : public ProbeComponent {
public:
    ReentrantMutationComponent(NeoEngine::ActorComponentWorld& world, NeoEngine::SceneEntity actor, ProbeComponent& dependency) : ProbeComponent(30U), world_(world), actor_(actor), dependency_(dependency) {}
    uint8_t TickGroup() const override { return 2U; }
    uint8_t TickDependencyCount() const override { return 1U; }
    uint16_t TickDependencyTypeId(uint8_t index) const override { return index == 0U ? 29U : 0U; }
    bool OnFixedTick(NeoEngine::SceneWorld&, NeoEngine::SceneEntity, uint32_t fixedTicks) override {
        tickedFixedTicks += fixedTicks;
        mutationRejected = dependency_.tickCalls > 0U && !world_.SetComponentEnabled(actor_, typeId_, false) && world_.LastError() == NeoEngine::ActorComponentError::MutationDuringDispatch;
        return mutationRejected;
    }
    NeoEngine::ActorComponentWorld& world_;
    NeoEngine::SceneEntity actor_{};
    ProbeComponent& dependency_;
    bool mutationRejected = false;
};

bool RunBeginPlayRollbackRetryRegression() {
    using namespace NeoEngine;
    auto scene = std::make_unique<SceneWorld>();
    auto world = std::make_unique<ActorComponentWorld>(*scene);
    SceneEntity actor{};
    if (!world->CreateActor(actor, "RollbackActor") || !scene->SetTransform(actor, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !world->AttachComponent(actor, std::make_unique<RejectEndPlayOnceComponent>()) || !world->AttachComponent(actor, std::make_unique<RejectBeginPlayOnceComponent>())) return false;
    if (world->BeginPlay() || world->LastError() != ActorComponentError::RollbackRejected) return false;
    return world->BeginPlay() && world->LastError() == ActorComponentError::None && world->EndPlay();
}

bool RunActivationRollbackRegression() {
    using namespace NeoEngine;
    auto scene = std::make_unique<SceneWorld>();
    auto world = std::make_unique<ActorComponentWorld>(*scene);
    SceneEntity actor{};
    if (!world->CreateActor(actor, "ActivationActor") || !scene->SetTransform(actor, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !world->AttachComponent(actor, std::make_unique<RejectDeactivationComponent>()) || !world->AttachComponent(actor, std::make_unique<RejectActivationComponent>())) return false;
    return !world->BeginPlay() && world->LastError() == ActorComponentError::RollbackRejected && world->ComponentCount() == 2U;
}

bool RunAttachCallbackRollbackRegression() {
    using namespace NeoEngine;
    auto scene = std::make_unique<SceneWorld>();
    auto world = std::make_unique<ActorComponentWorld>(*scene);
    SceneEntity actor{};
    if (!world->CreateActor(actor, "AttachActor") || !scene->SetTransform(actor, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !world->BeginPlay()) return false;
    if (world->AttachComponent(actor, std::make_unique<RejectAttachBeginPlayComponent>()) || world->LastError() != ActorComponentError::BeginPlayRejected || world->ComponentCount() != 0U) return false;
    if (world->AttachComponent(actor, std::make_unique<RejectAttachDetachComponent>()) || world->LastError() != ActorComponentError::RollbackRejected || world->ComponentCount() != 0U) return false;
    return true;
}

bool RunBeginPlayExceptionRegression() {
    using namespace NeoEngine;
    auto scene = std::make_unique<SceneWorld>();
    auto world = std::make_unique<ActorComponentWorld>(*scene);
    SceneEntity actor{};
    if (!world->CreateActor(actor, "BeginActor") || !scene->SetTransform(actor, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !world->AttachComponent(actor, std::make_unique<ThrowingBeginPlayComponent>())) return false;
    if (world->BeginPlay() || world->LastError() != ActorComponentError::BeginPlayRejected || world->ActorCount() != 1U || world->ComponentCount() != 1U || !world->SetComponentEnabled(actor, 43U, false)) return false;
    auto snapshot = std::make_unique<ActorComponentWorldSnapshot>();
    return world->CaptureSnapshot(*snapshot) && !snapshot->begunPlay && snapshot->actors.size() == 1U && !snapshot->actors[0].begunPlay;
}

bool RunTickMetadataExceptionRegression() {
    using namespace NeoEngine;
    auto scene = std::make_unique<SceneWorld>();
    auto world = std::make_unique<ActorComponentWorld>(*scene);
    SceneEntity actor{};
    if (!world->CreateActor(actor, "MetadataActor") || !world->AttachComponent(actor, std::make_unique<ThrowingTickMetadataComponent>()) || !world->BeginPlay()) return false;
    ActorComponentWorldReceipt receipt{9U, 8U, 7U, 6U};
    return !world->TickFixed(1U, receipt) && world->LastError() == ActorComponentError::TickRejected && receipt.actorCount == 9U && receipt.componentCount == 8U && receipt.tickedComponents == 7U && receipt.registrationRevision == 6U && world->EndPlay();
}

bool RunSnapshotMetadataExceptionRegression() {
    using namespace NeoEngine;
    auto scene = std::make_unique<SceneWorld>();
    auto world = std::make_unique<ActorComponentWorld>(*scene);
    SceneEntity actor{};
    if (!world->CreateActor(actor, "SnapshotMetadataActor") || !world->AttachComponent(actor, std::make_unique<ThrowingSnapshotMetadataComponent>())) return false;
    ActorComponentWorldSnapshot snapshot{};
    snapshot.begunPlay = true;
    return !world->CaptureSnapshot(snapshot) && world->LastError() == ActorComponentError::SnapshotRejected && snapshot.begunPlay && snapshot.actors.empty() && snapshot.componentBytes.empty();
}
}

int main() {
    using namespace NeoEngine;
    SceneWorld scene;
    ActorComponentWorld actors(scene);
    SceneEntity hero{};
    if (!actors.CreateActor(hero, "Hero") || !actors.IsActorAlive(hero) || actors.ActorCount() != 1U || actors.ComponentCount() != 0U) return 1;
    if (actors.ActorName(hero) == nullptr || *actors.ActorName(hero) != "Hero") return 2;
    if (!scene.SetTransform(hero, {10.0F, 0.0F, 2.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 3;

    uint32_t movementDetachCount = 0U;
    uint32_t movementEndPlayCount = 0U;
    auto movement = std::make_unique<ProbeComponent>(10U, &movementDetachCount, &movementEndPlayCount);
    ProbeComponent* movementView = movement.get();
    if (!actors.AttachComponent(hero, std::move(movement)) || movementView->attachCount != 1U || actors.ComponentCount(hero) != 1U) return 4;
    if (actors.AttachComponent(hero, std::make_unique<ProbeComponent>(10U)) || actors.LastError() != ActorComponentError::DuplicateComponent) return 5;
    uint32_t renderDetachCount = 0U;
    uint32_t renderEndPlayCount = 0U;
    auto render = std::make_unique<ProbeComponent>(11U, &renderDetachCount, &renderEndPlayCount);
    ProbeComponent* renderView = render.get();
    if (!actors.AttachComponent(hero, std::move(render)) || actors.ComponentCount(hero) != 2U) return 6;
    std::vector<SceneEntity> actorQuery;
    std::vector<uint16_t> componentQuery;
    ActorComponentWorldSnapshot structuralSnapshot{};
    movementView->snapshotValue = 42U;
    movementView->snapshotWorld_ = &actors;
    movementView->snapshotActor_ = hero;
    if (!actors.CollectActors(actorQuery) || actorQuery.size() != 1U || actorQuery[0] != hero || !actors.CollectComponentTypes(hero, componentQuery) || componentQuery.size() != 2U || componentQuery[0] != 10U || componentQuery[1] != 11U || !actors.CaptureSnapshot(structuralSnapshot) || structuralSnapshot.actors.size() != 1U || structuralSnapshot.begunPlay || structuralSnapshot.componentBytes.size() != sizeof(uint32_t) || structuralSnapshot.actors[0].snapshotSizes[0] != sizeof(uint32_t) || structuralSnapshot.actors[0].snapshotOffsets[0] != 0U || std::memcmp(structuralSnapshot.componentBytes.data(), &movementView->snapshotValue, sizeof(uint32_t)) != 0) return 7;
    if (!actors.BeginPlay() || !structuralSnapshot.actors.empty() && structuralSnapshot.actors[0].begunPlay || movementView->beginPlayCount != 1U || renderView->beginPlayCount != 1U || movementView->activateCount != 1U || renderView->activateCount != 1U) return 8;
    if (!actors.CaptureSnapshot(structuralSnapshot) || !structuralSnapshot.begunPlay || structuralSnapshot.actors[0].begunPlay != true || structuralSnapshot.componentBytes.size() != sizeof(uint32_t)) return 9;
    if (actors.AttachComponent(hero, std::make_unique<RejectActivationComponent>()) || actors.LastError() != ActorComponentError::ActivationRejected || actors.ComponentCount(hero) != 2U) return 9;
    movementView->snapshotValue = 99U;
    if (!actors.SetComponentEnabled(hero, 10U, false) || actors.IsComponentEnabled(hero, 10U)) return 10;
    ActorComponentWorldSnapshot invalidSnapshot = structuralSnapshot;
    invalidSnapshot.actors[0].snapshotOffsets[0] = kMaxActorComponentWorldSnapshotBytes;
    if (actors.RestoreSnapshot(invalidSnapshot) || actors.LastError() != ActorComponentError::RestoreRejected || movementView->snapshotValue != 99U || actors.IsComponentEnabled(hero, 10U)) return 11;
    invalidSnapshot = structuralSnapshot;
    invalidSnapshot.actors[0].componentTypeNames[0] = "DifferentComponent";
    if (actors.RestoreSnapshot(invalidSnapshot) || actors.LastError() != ActorComponentError::RestoreRejected || movementView->snapshotValue != 99U || actors.IsComponentEnabled(hero, 10U)) return 12;
    invalidSnapshot = structuralSnapshot;
    invalidSnapshot.begunPlay = false;
    if (actors.RestoreSnapshot(invalidSnapshot) || actors.LastError() != ActorComponentError::RestoreRejected || movementView->snapshotValue != 99U || actors.IsComponentEnabled(hero, 10U)) return 12;
    if (!actors.RestoreSnapshot(structuralSnapshot) || movementView->snapshotValue != 42U || !actors.IsComponentEnabled(hero, 10U) || !actors.IsComponentActive(hero, 11U)) return 12;
    if (!actors.SetComponentActive(hero, 11U, false) || actors.IsComponentActive(hero, 11U) || renderView->deactivateCount != 1U || !actors.SetComponentActive(hero, 11U, true) || !actors.IsComponentActive(hero, 11U) || renderView->activateCount != 2U) return 10;
    if (!actors.AttachComponent(hero, std::make_unique<RejectDeactivationComponent>()) || actors.SetComponentActive(hero, 22U, false) || actors.LastError() != ActorComponentError::DeactivationRejected || !actors.IsComponentActive(hero, 22U) || !actors.DetachComponent(hero, 22U)) return 10;
    if (!actors.SetComponentEnabled(hero, 10U, false) || actors.IsComponentEnabled(hero, 10U)) return 11;

    ActorComponentWorldReceipt receipt{};
    if (!actors.TickFixed(3U, receipt) || receipt.tickedComponents != 1U || renderView->tickedFixedTicks != 3U || movementView->tickedFixedTicks != 0U) return 12;
    if (!actors.SetComponentEnabled(hero, 10U, true) || !actors.TickFixed(2U, receipt) || receipt.tickedComponents != 2U || movementView->tickedFixedTicks != 2U || renderView->tickedFixedTicks != 5U) return 13;
    const ActorComponentWorldReceipt preservedTickReceipt = receipt;
    if (actors.TickFixed(0U, receipt) || actors.LastError() != ActorComponentError::TickRejected || receipt.tickedComponents != preservedTickReceipt.tickedComponents) return 14;
    if (actors.TickFixed(ActorComponentWorld::kMaxFixedTicks + 1U, receipt) || actors.LastError() != ActorComponentError::TickRejected || receipt.tickedComponents != preservedTickReceipt.tickedComponents) return 14;
    if (!actors.DetachComponent(hero, 10U) || movementDetachCount != 1U || movementEndPlayCount != 1U || actors.ComponentCount(hero) != 1U) return 14;
    if (actors.DetachComponent(hero, 10U) || actors.LastError() != ActorComponentError::InvalidComponent) return 15;

    SceneEntity child{};
    if (!actors.CreateActor(child, "Child") || !scene.SetTransform(child, {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !scene.SetParent(child, hero)) return 16;
    const Transform3* childWorld = scene.GetTransform(child);
    if (childWorld == nullptr || childWorld->x != 11.0F) return 17;
    auto childComponent = std::make_unique<ProbeComponent>(12U);
    ProbeComponent* childView = childComponent.get();
    if (!actors.AttachComponent(child, std::move(childComponent)) || childView->beginPlayCount != 1U || !actors.TickFixed(1U, receipt) || receipt.tickedComponents != 2U || renderView->tickedFixedTicks != 6U || childView->tickedFixedTicks != 1U) return 18;

    if (!actors.DestroyActor(hero) || actors.IsActorAlive(hero) || actors.ActorCount() != 1U || actors.ComponentCount() != 1U || renderDetachCount != 1U || renderEndPlayCount != 1U) return 19;
    if (actors.IsActorAlive(child) == false || scene.GetTransform(child) == nullptr) return 20;
    SceneEntity replacement{};
    if (!actors.CreateActor(replacement, "Replacement") || replacement.index != hero.index || replacement.generation == hero.generation || actors.IsActorAlive(hero)) return 21;

    if (!actors.AttachComponent(child, std::make_unique<RejectDetachComponent>()) || actors.ComponentCount(child) != 2U) return 22;
    if (actors.DestroyActor(child) || actors.LastError() != ActorComponentError::DetachRejected || !actors.IsActorAlive(child) || actors.ComponentCount() != 2U) return 23;
    if (actors.DetachComponent(child, 20U) || actors.LastError() != ActorComponentError::DetachRejected || actors.FindComponent(child, 20U) == nullptr) return 24;
    if (actors.AttachComponent(hero, std::make_unique<ProbeComponent>(30U)) || actors.LastError() != ActorComponentError::InvalidActor) return 25;
    auto dependency = std::make_unique<ProbeComponent>(29U);
    ProbeComponent* dependencyView = dependency.get();
    if (!actors.AttachComponent(replacement, std::move(dependency))) return 26;
    auto reentrant = std::make_unique<ReentrantMutationComponent>(actors, replacement, *dependencyView);
    ReentrantMutationComponent* reentrantView = reentrant.get();
    if (!actors.AttachComponent(replacement, std::move(reentrant)) || !actors.TickFixed(1U, receipt) || !reentrantView->mutationRejected || dependencyView->tickCalls != 1U || receipt.tickedComponents != 4U || actors.IsComponentEnabled(replacement, 30U) == false) return 27;
    if (!actors.AttachComponent(replacement, std::make_unique<MissingDependencyComponent>())) return 28;
    const ActorComponentWorldReceipt beforeRejectedTick = receipt;
    if (actors.TickFixed(1U, receipt) || actors.LastError() != ActorComponentError::DependencyRejected || receipt.tickedComponents != beforeRejectedTick.tickedComponents || actors.FindComponent(replacement, 31U) == nullptr) return 29;

    SceneWorld restoreScene;
    ActorComponentWorld restoreWorld(restoreScene);
    SceneEntity restoreActor{};
    if (!restoreWorld.CreateActor(restoreActor, "RestoreActor") || !restoreScene.SetTransform(restoreActor, {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 30;
    auto failOnce = std::make_unique<FailOnceRestoreComponent>();
    FailOnceRestoreComponent* failOnceView = failOnce.get();
    if (!restoreWorld.AttachComponent(restoreActor, std::move(failOnce)) || !restoreWorld.BeginPlay()) return 30;
    failOnceView->snapshotValue = 7U;
    ActorComponentWorldSnapshot restoreSnapshot{};
    if (!restoreWorld.CaptureSnapshot(restoreSnapshot) || restoreSnapshot.componentBytes.size() != sizeof(uint32_t)) return 30;
    failOnceView->snapshotValue = 99U;
    if (restoreWorld.RestoreSnapshot(restoreSnapshot) || restoreWorld.LastError() != ActorComponentError::RestoreRejected || failOnceView->snapshotValue != 99U) return 30;
    SceneWorld throwingScene;
    ActorComponentWorld throwingWorld(throwingScene);
    SceneEntity throwingActor{};
    if (!throwingWorld.CreateActor(throwingActor, "ThrowingActor") || !throwingScene.SetTransform(throwingActor, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !throwingWorld.AttachComponent(throwingActor, std::make_unique<ThrowingTickComponent>()) || !throwingWorld.BeginPlay()) return 31;
    ActorComponentWorldReceipt throwingReceipt{9U, 8U, 7U, 6U};
    if (throwingWorld.TickFixed(1U, throwingReceipt) || throwingWorld.LastError() != ActorComponentError::TickRejected || throwingReceipt.actorCount != 9U || throwingReceipt.componentCount != 8U || throwingReceipt.tickedComponents != 7U || throwingReceipt.registrationRevision != 6U) return 31;
    SceneWorld rollbackScene;
    ActorComponentWorld rollbackWorld(rollbackScene);
    SceneEntity rollbackActor{};
    if (!rollbackWorld.CreateActor(rollbackActor, "RollbackActor") || !rollbackScene.SetTransform(rollbackActor, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 32;
    auto failAlways = std::make_unique<FailAlwaysRestoreComponent>();
    FailAlwaysRestoreComponent* failAlwaysView = failAlways.get();
    if (!rollbackWorld.AttachComponent(rollbackActor, std::move(failAlways)) || !rollbackWorld.BeginPlay()) return 32;
    ActorComponentWorldSnapshot rollbackSnapshot{};
    if (!rollbackWorld.CaptureSnapshot(rollbackSnapshot)) return 32;
    failAlwaysView->snapshotValue = 99U;
    if (rollbackWorld.RestoreSnapshot(rollbackSnapshot) || rollbackWorld.LastError() != ActorComponentError::RollbackRejected || failAlwaysView->snapshotValue != 99U) return 32;
    SceneWorld retryScene;
    ActorComponentWorld retryWorld(retryScene);
    SceneEntity retryActor{};
    if (!retryWorld.CreateActor(retryActor, "RetryActor") || !retryScene.SetTransform(retryActor, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 33;
    if (!retryWorld.AttachComponent(retryActor, std::make_unique<RejectEndPlayOnceComponent>()) || !retryWorld.BeginPlay()) return 33;
    if (retryWorld.EndPlay() || retryWorld.LastError() != ActorComponentError::EndPlayRejected) return 33;
    ActorComponentWorldSnapshot endedSnapshot{};
    if (!retryWorld.EndPlay() || !retryWorld.CaptureSnapshot(endedSnapshot) || endedSnapshot.begunPlay || endedSnapshot.actors.size() != 1U || endedSnapshot.actors[0].begunPlay) return 33;
    if (!RunBeginPlayRollbackRetryRegression() || !RunActivationRollbackRegression() || !RunAttachCallbackRollbackRegression() || !RunBeginPlayExceptionRegression() || !RunTickMetadataExceptionRegression() || !RunSnapshotMetadataExceptionRegression()) return 34;
    return 0;
}
