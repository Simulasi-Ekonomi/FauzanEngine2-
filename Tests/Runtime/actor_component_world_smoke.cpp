#include "Runtime/ActorComponentWorld.h"

#include <cstdint>
#include <memory>
#include <string>

namespace {
class ProbeComponent : public NeoEngine::IActorComponent {
public:
    explicit ProbeComponent(uint16_t typeId, uint32_t* detachSink = nullptr) : typeId_(typeId), detachSink_(detachSink) {}
    uint16_t TypeId() const override { return typeId_; }
    bool OnAttach(NeoEngine::SceneWorld& world, NeoEngine::SceneEntity actor) override { ++attachCount; return world.GetTransform(actor) != nullptr; }
    bool OnDetach(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { ++detachCount; if (detachSink_ != nullptr) ++*detachSink_; return true; }
    bool OnFixedTick(NeoEngine::SceneWorld&, NeoEngine::SceneEntity, uint32_t fixedTicks) override { tickedFixedTicks += fixedTicks; ++tickCalls; return true; }
    uint16_t typeId_ = 0U;
    uint32_t* detachSink_ = nullptr;
    uint32_t attachCount = 0U;
    uint32_t detachCount = 0U;
    uint32_t tickCalls = 0U;
    uint32_t tickedFixedTicks = 0U;
};

class RejectDetachComponent final : public ProbeComponent {
public:
    RejectDetachComponent() : ProbeComponent(20U) {}
    bool OnDetach(NeoEngine::SceneWorld&, NeoEngine::SceneEntity) override { return false; }
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
    auto movement = std::make_unique<ProbeComponent>(10U, &movementDetachCount);
    ProbeComponent* movementView = movement.get();
    if (!actors.AttachComponent(hero, std::move(movement)) || movementView->attachCount != 1U || actors.ComponentCount(hero) != 1U) return 4;
    if (actors.AttachComponent(hero, std::make_unique<ProbeComponent>(10U)) || actors.LastError() != ActorComponentError::DuplicateComponent) return 5;
    uint32_t renderDetachCount = 0U;
    auto render = std::make_unique<ProbeComponent>(11U, &renderDetachCount);
    ProbeComponent* renderView = render.get();
    if (!actors.AttachComponent(hero, std::move(render)) || actors.ComponentCount(hero) != 2U) return 6;
    if (!actors.SetComponentEnabled(hero, 10U, false) || actors.IsComponentEnabled(hero, 10U)) return 7;

    ActorComponentWorldReceipt receipt{};
    if (!actors.TickFixed(3U, receipt) || receipt.tickedComponents != 1U || renderView->tickedFixedTicks != 3U || movementView->tickedFixedTicks != 0U) return 8;
    if (!actors.SetComponentEnabled(hero, 10U, true) || !actors.TickFixed(2U, receipt) || receipt.tickedComponents != 2U || movementView->tickedFixedTicks != 2U || renderView->tickedFixedTicks != 5U) return 9;
    if (actors.TickFixed(0U, receipt) || actors.LastError() != ActorComponentError::TickRejected) return 10;
    if (!actors.DetachComponent(hero, 10U) || movementDetachCount != 1U || actors.ComponentCount(hero) != 1U) return 11;
    if (actors.DetachComponent(hero, 10U) || actors.LastError() != ActorComponentError::InvalidComponent) return 12;

    SceneEntity child{};
    if (!actors.CreateActor(child, "Child") || !scene.SetTransform(child, {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !scene.SetParent(child, hero)) return 13;
    const Transform3* childWorld = scene.GetTransform(child);
    if (childWorld == nullptr || childWorld->x != 11.0F) return 14;
    auto childComponent = std::make_unique<ProbeComponent>(12U);
    ProbeComponent* childView = childComponent.get();
    if (!actors.AttachComponent(child, std::move(childComponent)) || !actors.TickFixed(1U, receipt) || receipt.tickedComponents != 2U || renderView->tickedFixedTicks != 6U || childView->tickedFixedTicks != 1U) return 15;

    if (!actors.DestroyActor(hero) || actors.IsActorAlive(hero) || actors.ActorCount() != 1U || actors.ComponentCount() != 1U || renderDetachCount != 1U) return 16;
    if (actors.IsActorAlive(child) == false || scene.GetTransform(child) == nullptr) return 17;
    SceneEntity replacement{};
    if (!actors.CreateActor(replacement, "Replacement") || replacement.index != hero.index || replacement.generation == hero.generation || actors.IsActorAlive(hero)) return 18;

    if (!actors.AttachComponent(child, std::make_unique<RejectDetachComponent>()) || actors.ComponentCount() != 2U) return 19;
    if (actors.DestroyActor(child) || actors.LastError() != ActorComponentError::DetachRejected || !actors.IsActorAlive(child) || actors.ComponentCount() != 2U) return 20;
    if (actors.DetachComponent(child, 20U) || actors.LastError() != ActorComponentError::DetachRejected || actors.FindComponent(child, 20U) == nullptr) return 21;
    if (actors.AttachComponent(hero, std::make_unique<ProbeComponent>(30U)) || actors.LastError() != ActorComponentError::InvalidActor) return 22;
    return 0;
}
