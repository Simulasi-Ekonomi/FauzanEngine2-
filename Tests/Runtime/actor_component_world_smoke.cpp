#include "Runtime/ActorComponentWorld.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace {
class ProbeComponent : public NeoEngine::IActorComponent {
public:
    explicit ProbeComponent(uint16_t typeId, uint32_t* detachSink = nullptr, uint32_t* endPlaySink = nullptr) : typeId_(typeId), detachSink_(detachSink), endPlaySink_(endPlaySink) {}
    uint16_t TypeId() const override { return typeId_; }
    bool OnAttach(NeoEngine::SceneWorld& world, NeoEngine::SceneEntity actor) override { ++attachCount; return world.GetTransform(actor) != nullptr; }
    bool OnDetach(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++detachCount; if (detachSink_ != nullptr) ++*detachSink_; return true; }
    bool OnBeginPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++beginPlayCount; return true; }
    bool OnEndPlay(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++endPlayCount; if (endPlaySink_ != nullptr) ++*endPlaySink_; return true; }
    uint8_t TickGroup() const override { return typeId_ == 11U ? 1U : 0U; }
    uint8_t TickOrder() const override { return typeId_ == 10U ? 1U : 0U; }
    uint16_t SnapshotSizeBytes() const override { return typeId_ == 10U ? sizeof(uint32_t) : 0U; }
    bool CaptureSnapshot(std::span<uint8_t> bytes) const override { if (typeId_ != 10U) return bytes.empty(); if (bytes.size() != sizeof(uint32_t)) return false; std::memcpy(bytes.data(), &snapshotValue, sizeof(snapshotValue)); return true; }
    uint32_t snapshotValue = 0U;
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
};

class RejectDetachComponent final : public ProbeComponent {
public:
    RejectDetachComponent() : ProbeComponent(20U) {}
    bool OnDetach(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { return false; }
};

class ReentrantMutationComponent final : public ProbeComponent {
public:
    ReentrantMutationComponent(NeoEngine::ActorComponentWorld& world, NeoEngine::SceneEntity actor) : ProbeComponent(30U), world_(world), actor_(actor) {}
    bool OnFixedTick(NeoEngine::SceneWorld&, NeoEngine::SceneEntity, uint32_t fixedTicks) override {
        tickedFixedTicks += fixedTicks;
        mutationRejected = !world_.SetComponentEnabled(actor_, typeId_, false) && world_.LastError() == NeoEngine::ActorComponentError::MutationDuringDispatch;
        return mutationRejected;
    }
    NeoEngine::ActorComponentWorld& world_;
    NeoEngine::SceneEntity actor_{};
    bool mutationRejected = false;
};
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
    if (!actors.CollectActors(actorQuery) || actorQuery.size() != 1U || actorQuery[0] != hero || !actors.CollectComponentTypes(hero, componentQuery) || componentQuery.size() != 2U || componentQuery[0] != 10U || componentQuery[1] != 11U || !actors.CaptureSnapshot(structuralSnapshot) || structuralSnapshot.actors.size() != 1U || structuralSnapshot.begunPlay || structuralSnapshot.componentBytes.size() != sizeof(uint32_t) || structuralSnapshot.actors[0].snapshotSizes[0] != sizeof(uint32_t) || structuralSnapshot.actors[0].snapshotOffsets[0] != 0U || std::memcmp(structuralSnapshot.componentBytes.data(), &movementView->snapshotValue, sizeof(uint32_t)) != 0) return 7;
    if (!actors.BeginPlay() || !structuralSnapshot.actors.empty() && structuralSnapshot.actors[0].begunPlay || movementView->beginPlayCount != 1U || renderView->beginPlayCount != 1U) return 8;
    if (!actors.CaptureSnapshot(structuralSnapshot) || !structuralSnapshot.begunPlay || structuralSnapshot.actors[0].begunPlay != true || structuralSnapshot.componentBytes.size() != sizeof(uint32_t)) return 9;
    if (!actors.SetComponentEnabled(hero, 10U, false) || actors.IsComponentEnabled(hero, 10U)) return 10;

    ActorComponentWorldReceipt receipt{};
    if (!actors.TickFixed(3U, receipt) || receipt.tickedComponents != 1U || renderView->tickedFixedTicks != 3U || movementView->tickedFixedTicks != 0U) return 11;
    if (!actors.SetComponentEnabled(hero, 10U, true) || !actors.TickFixed(2U, receipt) || receipt.tickedComponents != 2U || movementView->tickedFixedTicks != 2U || renderView->tickedFixedTicks != 5U) return 12;
    if (actors.TickFixed(0U, receipt) || actors.LastError() != ActorComponentError::TickRejected) return 13;
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
    auto reentrant = std::make_unique<ReentrantMutationComponent>(actors, replacement);
    ReentrantMutationComponent* reentrantView = reentrant.get();
    if (!actors.AttachComponent(replacement, std::move(reentrant)) || !actors.TickFixed(1U, receipt) || !reentrantView->mutationRejected || receipt.tickedComponents != 3U || actors.IsComponentEnabled(replacement, 30U) == false) return 26;
    return 0;
}
